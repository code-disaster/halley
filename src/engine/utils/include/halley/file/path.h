#pragma once

#include "halley/text/halleystring.h"
#include "halley/utils/utils.h"
#include <gsl/span>

namespace Halley
{
	class Path
	{
	public:
		Path();
		Path(const char* name);
		Path(const std::string& name);
		Path(const String& name);

		Path(const Path& other) = default;
		Path(Path&& other) noexcept = default;
		Path& operator=(const Path& other) = default;
		Path& operator=(Path&& other) noexcept = default;

		Path& operator=(const std::string& other);
		Path& operator=(const String& other);

		Path getRoot() const;
		Path getFront(size_t n) const;
		Path getFilename() const;
		Path getStem() const;
		String getExtension() const;
		String getString() const;
		String getNativeString() const;
		String toString() const;

		size_t getNumberPaths() const;

		Path dropFront(int numberFolders) const;

		Path parentPath() const;
		Path replaceExtension(String newExtension) const;

		Path operator/(const char* other) const;
		Path operator/(const Path& other) const;
		Path operator/(const String& other) const;
		Path operator/(const std::string& other) const;

		bool operator==(const char* other) const;
		bool operator==(const String& other) const;
		bool operator==(const Path& other) const;

		bool operator!=(const Path& other) const;
		bool operator<(const Path& other) const;

		std::string string() const;

		static void writeFile(const Path& path, gsl::span<const gsl::byte> data);
		static void writeFile(const Path& path, const Bytes& data);
		static void writeFile(const Path& path, const String& data);
		static Bytes readFile(const Path& path);
		static void removeFile(const Path& path);

		Path makeRelativeTo(const Path& path) const;
		Path changeRelativeRoot(const Path& currentParent, const Path& newParent) const;

		bool isDirectory() const;
		bool isFile() const;
		bool isAbsolute() const;
		bool isEmpty() const;

	private:
		Vector<String> pathParts;
		void normalise();
		void setPath(const String& value);

		explicit Path(Vector<String> parts);
	};

	using TimestampedPath = std::pair<Path, int64_t>;
}
