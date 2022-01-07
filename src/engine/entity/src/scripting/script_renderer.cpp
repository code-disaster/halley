#include "scripting/script_renderer.h"
#include "world.h"
#include "halley/core/graphics/painter.h"
#include "halley/maths/bezier.h"
#include "halley/support/logger.h"
#include "scripting/script_graph.h"
#include "scripting/script_node_type.h"
using namespace Halley;

#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "components/transform_2d_component.h"

ScriptRenderer::ScriptRenderer(Resources& resources, World& world, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom)
	: resources(resources)
	, world(world)
	, nodeTypeCollection(nodeTypeCollection)
	, nativeZoom(nativeZoom)
{
	nodeBg = Sprite().setImage(resources, "halley_ui/ui_float_solid_window.png").setPivot(Vector2f(0.5f, 0.5f));
	variableBg = Sprite().setImage(resources, "halley_ui/script_variable.png").setPivot(Vector2f(0.5f, 0.5f));
	pinSprite = Sprite().setImage(resources, "halley_ui/ui_render_graph_node_pin.png").setPivot(Vector2f(0.5f, 0.5f));
	labelText
		.setFont(resources.get<Font>("Ubuntu Bold"))
		.setSize(14)
		.setColour(Colour(1, 1, 1))
		.setOutlineColour(Colour(0, 0, 0))
		.setOutline(1)
		.setAlignment(0.5f);
}

void ScriptRenderer::setGraph(const ScriptGraph* graph)
{
	this->graph = graph;
}

void ScriptRenderer::setState(ScriptState* scriptState)
{
	state = scriptState;
}

void ScriptRenderer::draw(Painter& painter, Vector2f basePos, float curZoom)
{
	if (!graph) {
		return;
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);

	for (size_t i = 0; i < graph->getNodes().size(); ++i) {
		drawNodeOutputs(painter, basePos, i, *graph, effectiveZoom);
	}

	if (currentPath) {
		drawConnection(painter, currentPath.value(), curZoom, false);
	}
	
	for (uint32_t i = 0; i < static_cast<uint32_t>(graph->getNodes().size()); ++i) {
		const auto& node = graph->getNodes()[i];

		const bool highlightThis = highlightNode && highlightNode->nodeId == i;
		const auto pinType = highlightThis ? highlightNode->element : std::optional<ScriptNodePinType>();
		const auto pinId = highlightThis ? highlightNode->elementId : 0;
		
		NodeDrawMode drawMode;
		if (state) {
			// Rendering in-game, with execution state
			const auto nodeIntrospection = state->getNodeIntrospection(i);
			if (nodeIntrospection.state == ScriptState::NodeIntrospectionState::Active) {
				drawMode.type = NodeDrawModeType::Active;
				drawMode.time = nodeIntrospection.time;
			} else if (nodeIntrospection.state == ScriptState::NodeIntrospectionState::Visited) {
				drawMode.type = NodeDrawModeType::Visited;
				drawMode.time = nodeIntrospection.time;
			}
			drawMode.activationTime = nodeIntrospection.activationTime;
		} else {
			// Rendering in editor
			if (highlightThis && highlightNode->element.type == ScriptNodeElementType::Node) {
				drawMode.type = NodeDrawModeType::Highlight;
			}
		}
		
		drawNode(painter, basePos, node, effectiveZoom, drawMode, pinType, pinId);
	}
}

void ScriptRenderer::drawNodeOutputs(Painter& painter, Vector2f basePos, size_t nodeIdx, const ScriptGraph& graph, float curZoom)
{
	const ScriptGraphNode& node = graph.getNodes().at(nodeIdx);
	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}

	const bool nodeHighlighted = highlightNode && highlightNode->nodeId == nodeIdx;

	for (size_t i = 0; i < node.getPins().size(); ++i) {
		const auto& srcPinType = nodeType->getPin(node, i);
		const auto& pin = node.getPins()[i];
		
		for (const auto& pinConnection: pin.connections) {
			std::optional<Vector2f> dstPos;
			ScriptNodePinType dstPinType;

			bool highlighted = nodeHighlighted;

			if (pinConnection.dstNode && srcPinType.direction == ScriptNodePinDirection::Output) {
				const size_t dstIdx = pinConnection.dstPin;
				const auto& dstNode = graph.getNodes().at(pinConnection.dstNode.value());
				const auto* dstNodeType = nodeTypeCollection.tryGetNodeType(dstNode.getType());
				if (!dstNodeType) {
					continue;
				}
				dstPos = getNodeElementArea(*dstNodeType, basePos, dstNode, dstIdx, curZoom).getCentre();
				dstPinType = dstNodeType->getPin(node, dstIdx);
				if (highlightNode && highlightNode->nodeId == pinConnection.dstNode.value()) {
					highlighted = true;
				}
			} else if (pinConnection.entity.isValid()) {
				auto entity = world.tryGetEntity(pinConnection.entity);
				if (entity.isValid()) {
					auto* transform = entity.tryGetComponent<Transform2DComponent>();
					if (transform) {
						dstPos = transform->getGlobalPosition();
						dstPinType = ScriptNodePinType{ ScriptNodeElementType::TargetPin, ScriptNodePinDirection::Output };
					}
				}
			}
			
			if (dstPos) {
				const Vector2f srcPos = getNodeElementArea(*nodeType, basePos, node, i, curZoom).getCentre();
				drawConnection(painter, ConnectionPath{ srcPos, dstPos.value(), srcPinType, dstPinType }, curZoom, highlighted);
			}
		}
	}
}

BezierCubic ScriptRenderer::makeBezier(const ConnectionPath& path) const
{
	auto getSideNormal = [] (ScriptPinSide side) -> Vector2f
	{
		switch (side) {
		case ScriptPinSide::Left:
			return Vector2f(-1, 0);
		case ScriptPinSide::Right:
			return Vector2f(1, 0);
		case ScriptPinSide::Top:
			return Vector2f(0, -1);
		case ScriptPinSide::Bottom:
			return Vector2f(0, 1);
		}
		return Vector2f();
	};
	
	const Vector2f fromDir = getSideNormal(path.fromType.getSide());
	const Vector2f toDir = getSideNormal(path.toType.getSide());

	const auto delta = path.to - path.from;
	const float dist = std::max(std::max(std::abs(delta.x), std::abs(delta.y)), 20.0f) / 2;

	return BezierCubic(path.from, path.from + dist * fromDir, path.to + dist * toDir, path.to);
}

void ScriptRenderer::drawConnection(Painter& painter, const ConnectionPath& path, float curZoom, bool highlight) const
{
	const auto bezier = makeBezier(path);
	const auto baseCol = getPinColour(path.fromType);
	const auto col = highlight ? baseCol.inverseMultiplyLuma(0.25f) : baseCol;
	painter.drawLine(bezier + Vector2f(1.0f, 2.0f) / curZoom, 3.0f / curZoom, Colour4f(0, 0, 0, 0.3f));
	painter.drawLine(bezier, 3.0f / curZoom, col);
}

void ScriptRenderer::drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom, NodeDrawMode drawMode, std::optional<ScriptNodePinType> highlightElement, uint8_t highlightElementId)
{
	const Vector2f border = Vector2f(18, 18);
	const Vector2f nodeSize = getNodeSize(curZoom);
	const auto pos = ((basePos + node.getPosition()) * curZoom).round() / curZoom;

	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}

	{
		const auto baseCol = getNodeColour(*nodeType);
		Colour4f col = baseCol;
		Colour4f iconCol = Colour4f(1, 1, 1);
		
		switch (drawMode.type) {
		case NodeDrawModeType::Highlight:
			col = col.inverseMultiplyLuma(0.5f);
			break;
		case NodeDrawModeType::Active:
			{
				const float phase = drawMode.time * 2.0f * pif();
				col = col.inverseMultiplyLuma(sinRange(phase, 0.3f, 1.0f));
				break;
			}
		case NodeDrawModeType::Visited:
			col = col.multiplyLuma(0.3f);
			iconCol = Colour4f(0.5f, 0.5f, 0.5f);
			break;
		}

		if (drawMode.activationTime > 0.0f) {
			const float t = drawMode.activationTime;
			col = lerp(col, Colour4f(1, 1, 1), t * t);
		}
		
		// Node body
		const bool variable = nodeType->getClassification() == ScriptNodeClassification::Variable;
		if (variable) {
			variableBg.clone()
				.setColour(col)
				.setPosition(pos)
				.setScale(1.0f / curZoom)
				.draw(painter);
		} else {
			nodeBg.clone()
				.setColour(col)
				.setPosition(pos)
				.scaleTo(nodeSize + border)
				.setSize(nodeBg.getSize() / curZoom)
				.setSliceScale(1.0f / curZoom)
				.draw(painter);
		}

		const auto label = nodeType->getLabel(node);
		const float iconExtraOffset = nodeType->getClassification() == ScriptNodeClassification::Variable ? -2.0f : 0.0f;
		const Vector2f iconOffset = label.isEmpty() ? Vector2f() : Vector2f(0, (-8.0f + iconExtraOffset) / curZoom).round();

		// Icon
		getIcon(*nodeType, node).clone()
			.setPosition(pos + iconOffset)
			.setScale(1.0f / curZoom)
			.setColour(iconCol)
			.draw(painter);

		// Label
		if (!label.isEmpty()) {
			labelText.clone()
				.setPosition(pos + Vector2f(0, (8.0f + iconExtraOffset) / curZoom).round())
				.setText(label)
				.setSize(14 / curZoom)
				.setOutline(8.0f / curZoom)
				.setOutlineColour(col.multiplyLuma(0.75f))
				.draw(painter);
		}
	}

	// Draw pins
	const auto& pins = nodeType->getPinConfiguration(node);
	for (size_t i = 0; i < pins.size(); ++i) {
		const auto& pinType = pins[i];
		const auto circle = getNodeElementArea(*nodeType, basePos, node, i, curZoom);
		const auto baseCol = getPinColour(pinType);
		const auto col = highlightElement == pinType && highlightElementId == i ? baseCol.inverseMultiplyLuma(0.3f) : baseCol;
		pinSprite.clone()
			.setPosition(circle.getCentre())
			.setColour(col)
			.setScale(1.0f / curZoom)
			.draw(painter);
	}
}

Vector2f ScriptRenderer::getNodeSize(float curZoom) const
{
	return Vector2f(60, 60);
}

Circle ScriptRenderer::getNodeElementArea(const IScriptNodeType& nodeType, Vector2f basePos, const ScriptGraphNode& node, size_t pinN, float curZoom) const
{
	const Vector2f nodeSize = getNodeSize(curZoom);
	const auto getOffset = [&] (size_t idx, size_t n)
	{
		const float spacing = nodeSize.x / (n + 1);
		return (static_cast<float>(idx) - (n - 1) * 0.5f) * spacing;
	};

	const auto& pin = nodeType.getPin(node, pinN);
	const auto pinSide = pin.getSide();
	
	size_t pinsOnSide = 0;
	size_t idxOnSide = 0;
	const auto& pins = nodeType.getPinConfiguration(node);
	for (size_t i = 0; i < pins.size(); ++i) {
		const auto& pinType = pins[i];
		if (i == pinN) {
			idxOnSide = pinsOnSide;
		}
		if (pinType.getSide() == pinSide) {
			++pinsOnSide;
		}
	}
	
	const auto sideOffset = getOffset(idxOnSide, pinsOnSide);
	Vector2f offset;
	switch (pinSide) {
	case ScriptPinSide::Left:
		offset = Vector2f(-nodeSize.x * 0.5f, sideOffset);
		break;
	case ScriptPinSide::Right:
		offset = Vector2f(nodeSize.x * 0.5f, sideOffset);
		break;
	case ScriptPinSide::Top:
		offset = Vector2f(sideOffset, -nodeSize.y * 0.5f);
		break;
	case ScriptPinSide::Bottom:
		offset = Vector2f(sideOffset, nodeSize.y * 0.5f);
		break;
	default:
		break;
	}
	
	const Vector2f pos = basePos + node.getPosition();
	const Vector2f centre = pos + offset / curZoom;
	const float radius = 4.0f / curZoom;
	
	return Circle(centre, radius);
}

Colour4f ScriptRenderer::getNodeColour(const IScriptNodeType& nodeType)
{
	switch (nodeType.getClassification()) {
	case ScriptNodeClassification::Terminator:
		return Colour4f(0.97f, 0.35f, 0.35f);
	case ScriptNodeClassification::Action:
		return Colour4f(0.07f, 0.84f, 0.09f);
	case ScriptNodeClassification::Variable:
		return Colour4f(0.91f, 0.71f, 0.0f);
	case ScriptNodeClassification::FlowControl:
		return Colour4f(0.35f, 0.35f, 0.97f);
	}
	return Colour4f(0.2f, 0.2f, 0.2f);
}

Colour4f ScriptRenderer::getPinColour(ScriptNodePinType pinType) const
{
	switch (pinType.type) {
	case ScriptNodeElementType::FlowPin:
		return Colour4f(0.75f, 0.75f, 0.99f);
	case ScriptNodeElementType::ReadDataPin:
		return Colour4f(0.91f, 0.55f, 0.2f);
	case ScriptNodeElementType::WriteDataPin:
		return Colour4f(0.91f, 0.2f, 0.2f);
	case ScriptNodeElementType::TargetPin:
		return Colour4f(0.35f, 1, 0.35f);
	}

	return Colour4f();
}

const Sprite& ScriptRenderer::getIcon(const IScriptNodeType& nodeType, const ScriptGraphNode& node)
{
	const auto& iconName = nodeType.getIconName(node);
	
	const auto iter = icons.find(iconName);
	if (iter != icons.end()) {
		return iter->second;
	}
	icons[iconName] = Sprite().setImage(resources, iconName).setPivot(Vector2f(0.5f, 0.5f));
	return icons[iconName];
}

std::optional<ScriptRenderer::NodeUnderMouseInfo> ScriptRenderer::getNodeUnderMouse(Vector2f basePos, float curZoom, std::optional<Vector2f> mousePos, bool pinPriority) const
{
	if (!graph || !mousePos) {
		return {};
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);
	const auto nodeSize = getNodeSize(effectiveZoom);
	const Rect4f area = Rect4f(-nodeSize / 2, nodeSize / 2) / effectiveZoom;

	float bestDistance = std::numeric_limits<float>::max();
	std::optional<NodeUnderMouseInfo> bestResult;
	
	for (size_t i = 0; i < graph->getNodes().size(); ++i) {
		const auto& node = graph->getNodes()[i];
		const auto pos = basePos + node.getPosition();

		const auto nodeBounds = Circle(pos, area.getSize().length() / 2);
		if (!nodeBounds.contains(mousePos.value())) {
			continue;
		}
		
		const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
		if (!nodeType) {
			continue;
		}
		const auto curRect = area + pos;
		
		// Check each pin handle
		bool foundPin = false;
		const auto& pins = nodeType->getPinConfiguration(node);
		for	(size_t j = 0; j < pins.size(); ++j) {
			const auto& pinType = pins[j];
			const auto circle = getNodeElementArea(*nodeType, basePos, node, j, curZoom).expand((pinPriority ? 12.0f : 4.0f) / curZoom);
			if (circle.contains(mousePos.value())) {
				foundPin = true;
				const float distance = (mousePos.value() - circle.getCentre()).length();
				if (distance < bestDistance) {
					bestDistance = distance;
					bestResult = NodeUnderMouseInfo{ static_cast<uint32_t>(i), pinType, static_cast<uint8_t>(j), curRect, circle.getCentre() };
				}
			}
		}
		
		// Check main body
		if (!foundPin && curRect.contains(mousePos.value())) {
			const float distance = (mousePos.value() - curRect.getCenter()).length();
			if (distance < bestDistance) {
				bestDistance = distance;
				bestResult = NodeUnderMouseInfo{ static_cast<uint32_t>(i), ScriptNodePinType{ScriptNodeElementType::Node}, 0, curRect, Vector2f() };
			}
		}
	}

	return bestResult;
}

void ScriptRenderer::setHighlight(std::optional<NodeUnderMouseInfo> node)
{
	highlightNode = node;
}

void ScriptRenderer::setCurrentPath(std::optional<ConnectionPath> path)
{
	currentPath = path;
}
