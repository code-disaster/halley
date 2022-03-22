#include "script_music.h"
using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptPlayMusic::getSettingTypes() const
{
	return { SettingType{ "music", "Halley::String", Vector<String>{""} } };
}

gsl::span<const IScriptNodeType::PinType> ScriptPlayMusic::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptPlayMusic::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Play music ");
	str.append(node.getSettings()["music"].asString(""), Colour4f(0.97f, 0.35f, 0.35f));
	return str.moveResults();
}

IScriptNodeType::Result ScriptPlayMusic::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	environment.playMusic(node.getSettings()["music"].asString(""), 1.0f);
	return Result(ScriptNodeExecutionState::Done);
}



gsl::span<const IScriptNodeType::PinType> ScriptStopMusic::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptStopMusic::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Stop playing music.");
	return str.moveResults();
}

IScriptNodeType::Result ScriptStopMusic::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	environment.stopMusic(1.0f);
	return Result(ScriptNodeExecutionState::Done);
}
