#pragma once
/*
 * Copyright 2010-2024 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma warning(push)
#pragma warning(disable : 4668 6011 6255 6293 6386 26439 26495 26498 26819)
#include "../../libs/rapidyaml/ryml.hpp"
#include "../../libs/rapidyaml/ryml_std.hpp"
#pragma warning(pop)
#include <unordered_map>

//hash function for ryml::csubstr for unordered_map -> just calls the same thing std::hash<std::string> does
template <> struct std::hash<ryml::csubstr>{ std::size_t operator()(const ryml::csubstr& k) const {	return _Hash_array_representation(k.str, k.len); } };

namespace OpenXcom
{

// Deserialization template for enums. We have to overload from_chars() instead of read(), because read() already has a template for all types -> template conflict
template <typename EnumType>
typename std::enable_if<std::is_enum<EnumType>::value, bool>::type inline from_chars(ryml::csubstr buf, EnumType* v) noexcept
{
	return ryml::atoi(buf, (int*)v);
}

template <typename EnumType>
typename std::enable_if<std::is_enum<EnumType>::value, size_t>::type inline to_chars(ryml::substr buf, EnumType v) noexcept
{
	return ryml::itoa(buf, (int)v);
}

namespace YAML
{

const std::string Null = "~"; // "~" or "null" or "Null" or "NULL"; serializing null is the same as serializing a string

void setGlobalErrorHandler();

class YamlRootNodeReader;
class YamlRootNodeWriter;

struct YamlString
{
	std::string yaml;
	YamlString(std::string yamlString);
};

class YamlNodeReader
{
protected:
	ryml::ConstNodeRef _node;
	const YamlRootNodeReader* _root;
	bool _invalid;
	std::unordered_map<ryml::csubstr, ryml::id_type>* _index;

	ryml::ConstNodeRef getChildNode(const ryml::csubstr& key) const;

public:
	YamlNodeReader(); // vector demands a default constructor despite it never being used
	YamlNodeReader(const YamlRootNodeReader* root, const ryml::ConstNodeRef& node);
	YamlNodeReader(const YamlRootNodeReader* root, const ryml::ConstNodeRef& node, bool useIndex);
	~YamlNodeReader();

	/// Returns a copy of the current mapping container with O(1) access to the children. O(n) is spent building the index.
	const YamlNodeReader useIndex() const;


	/// Deserializes the value of the found child into the outputValue. If the node is invalid or the key doesn't exist, outputValue is set to defaultValue.
	template <typename OutputType>
	void readN(ryml::csubstr key, OutputType& outputValue, const OutputType& defaultValue) const;

	/// Returns a deserialized key of the current node. Throws if the node is invalid or itself has no key.
	template <typename OutputType>
	OutputType readKey() const;

	/// Returns a deserialized key of the current node, or a default value if the node is invalid or itself has no key
	template <typename OutputType>
	OutputType readKey(const OutputType& defaultValue) const;

	/// Returns a deserialized value of the current node. Throws if the node is invalid.
	template <typename OutputType>
	OutputType readVal() const;

	/// Returns a deserialized value of the current node, or a default value if the node is invalid
	template <typename OutputType>
	OutputType readVal(const OutputType& defaultValue) const;

	/// Returns a deserialized binary value of the current node. Throws if the node is invalid.
	std::vector<char> readValBase64() const;


	/// Returns false if the node is invalid or the key doesn't exist. Otherwise returns true and deserializes the value of the found child into the outputValue.
	template <typename OutputType>
	bool tryRead(ryml::csubstr key, OutputType& outputValue) const;

	/// Returns false if the node is invalid or itself has no key. Otherwise returns true and deserializes the key of the current node into the outputValue.
	template <typename OutputType>
	bool tryReadKey(OutputType& outputValue) const;

	/// Returns false if the node is invalid. Otherwise returns true and deserializes the value of the current node into the outputValue.
	template <typename OutputType>
	bool tryReadVal(OutputType& outputValue) const;


	/// Returns the number of children of the current node. O(n) complexity.
	size_t childrenCount() const;

	/// Builds a vector of children and retuns it
	std::vector<YamlNodeReader> children() const;

	bool isValid() const;
	bool isMap() const;
	bool isSeq() const;
	bool hasVal() const;
	bool hasNullVal() const;
	bool hasValTag() const;

	/// Returns true if the node is valid, has a tag, and the tag is a core tag
	bool hasValTag(ryml::YamlTag_e tag) const;
	/// Returns true if the node is valid, has a tag, and the tag's name equals tagName
	bool hasValTag(std::string tagName) const;
	/// Returns node's value's tag, or an empty string if there is none
	std::string getValTag() const;

	/// Serializes the node and its descendants to a YamlString
	const YamlString emit() const;
	/// Serializes the node's descendants to a YamlString
	const YamlString emitDescendants() const;

	ryml::Location getLocationInFile() const;

	/// Returns a child in the current mapping container or an invalid child. Throws if it's not a mapping container.
	const YamlNodeReader operator[](ryml::csubstr key) const;
	/// Returns a child at a specific position or an invalid child.
	const YamlNodeReader operator[](size_t pos) const;
	/// Returns whether the current node is valid
	explicit operator bool() const;

	friend YamlRootNodeReader;
	friend YamlRootNodeWriter;
};

class YamlRootNodeReader : public YamlNodeReader
{
private:
	ryml::Tree* _tree;
	ryml::Parser* _parser;
	ryml::EventHandlerTree* _eventHandler;
	std::string _fileName;

	ryml::Location getLocationInFile(const ryml::ConstNodeRef& node) const;

	void Parse(const ryml::csubstr& yaml, std::string fileName, bool withNodeLocations);

public:
	YamlRootNodeReader(std::string fullFilePath, bool onlyInfoHeader);
	YamlRootNodeReader(char* data, size_t size, std::string fileNameForError);
	YamlRootNodeReader(const YamlString& yamlString, std::string description);
	~YamlRootNodeReader();

	/// Returns base class to avoid slicing
	YamlNodeReader sansRoot() const;

	friend YamlNodeReader;
	friend YamlRootNodeWriter;
};

class YamlNodeWriter
{
protected:
	const YamlRootNodeWriter* _root;
	ryml::NodeRef _node;

public:
	YamlNodeWriter(const YamlRootNodeWriter* root, ryml::NodeRef node);

	/// Converts writer to a reader
	YamlNodeReader toReader();

	/// Adds a container child to the current sequence container
	YamlNodeWriter write();
	/// Adds a container child to the current mapping container
	YamlNodeWriter operator[](ryml::csubstr key);
	/// Adds a scalar value child to the current sequence container, serializing the provided value
	template <typename InputType>
	YamlNodeWriter write(const InputType& inputValue);
	/// Adds a scalar value child to the current mapping container, serializing the provided value
	template <typename InputType>
	YamlNodeWriter write(ryml::csubstr key, const InputType& inputValue);
	/// If the inputVector is not empty, adds a sequence container child to the current mapping container.
	/// The callback (YamlNodeWriter w, InputType val) should specify how to write a vector element to the sequence container.
	template <typename InputType, typename Func>
	void write(ryml::csubstr key, const std::vector<InputType>& inputVector, Func callback);
	/// Adds a scalar value child to the current mapping container, serializing the provided binary data
	YamlNodeWriter writeBase64(ryml::csubstr key, char* data, size_t size);
	/// Adds a value to the current node.
	template <typename InputType>
	void setValue(const InputType& inputValue);

	/// Marks the current node as a mapping container
	void setAsMap();
	/// Marks the current node as a sequence container
	void setAsSeq();
	/// Marks the current node to serialize as single-line flow-style
	void setFlowStyle();
	/// Marks the current node to serialize as multi-line block-style
	void setBlockStyle();
	/// Marks the current node to 
	void setAsQuoted();

	void unsetAsMap();
	void unsetAsSeq();

	/// Saves a string to the internal buffer. In a rare case when a key isn't a string literal, this ensures its lifetime until the serialization is done.
	ryml::csubstr saveString(const std::string& string);

	/// Emits a yaml string based on the current node and its subtree
	YamlString emit();

	friend YamlRootNodeReader;
	friend YamlRootNodeWriter;
};

class YamlRootNodeWriter : public YamlNodeWriter
{
private:
	ryml::Tree* _tree;
	ryml::Parser* _parser;
	ryml::EventHandlerTree* _eventHandler;

public:
	YamlRootNodeWriter();
	YamlRootNodeWriter(size_t bufferCapacity);
	~YamlRootNodeWriter();

	/// Returns base class to avoid slicing
	YamlNodeWriter sansRoot();

	friend YamlNodeReader;
	friend YamlNodeWriter;
	friend YamlRootNodeReader;
};

/* Template implementations below  */

template <typename OutputType>
void YamlNodeReader::readN(ryml::csubstr key, OutputType& outputValue, const OutputType& defaultValue) const
{
	if (!tryRead(key, outputValue))
		outputValue = defaultValue;
}

template <typename OutputType>
OutputType YamlNodeReader::readKey() const
{
	OutputType output;
	if (!tryReadKey(output))
		throw std::runtime_error("Tried to deserialize invalid node's key!");
	return output;
}

template <typename OutputType>
inline OutputType YamlNodeReader::readKey(const OutputType& defaultValue) const
{
	OutputType output;
	if (!tryReadKey(output))
		output = defaultValue;
	return output;
}

template <typename OutputType>
OutputType YamlNodeReader::readVal() const
{
	OutputType output;
	if (!tryReadVal(output))
		throw std::runtime_error("Tried to deserialize invalid node!");
	return output;
}

template <typename OutputType>
OutputType YamlNodeReader::readVal(const OutputType& defaultValue) const
{
	OutputType output;
	if (!tryReadVal(output))
		output = defaultValue;
	return output;
}

template <typename OutputType>
bool YamlNodeReader::tryRead(ryml::csubstr key, OutputType& outputValue) const
{
	if (_invalid)
		return false;
	if (!_index)
	{
		if (!_node.is_map())
			return false;
		const auto& child = _node.find_child(key);
		if (child.invalid())
			return false;
		if (!read(child, &outputValue))
			ryml::error(_node.tree()->callbacks(), "Could not deserialize value!", 29, getLocationInFile());
		return true;
	}
	if (!_index->count(key))
		return false;
	if (!read(_node.tree()->cref(_index->at(key)), &outputValue))
		ryml::error(_node.tree()->callbacks(), "Could not deserialize value!", 29, getLocationInFile());
	return true;
}

template <typename OutputType>
bool YamlNodeReader::tryReadKey(OutputType& outputValue) const
{
	if (_invalid || !_node.has_key())
		return false;
	_node >> ryml::key(outputValue);
	return true;
}

template <typename OutputType>
bool YamlNodeReader::tryReadVal(OutputType& outputValue) const
{
	if (_invalid)
		return false;
	if (!read(_node, &outputValue))
		ryml::error(_node.tree()->callbacks(), "Could not deserialize value!", 29, getLocationInFile());
	return true;
}

template <typename InputType>
inline YamlNodeWriter YamlNodeWriter::write(const InputType& inputValue)
{
	return YamlNodeWriter(_root, _node.append_child() << inputValue);
}

template <typename InputType>
YamlNodeWriter YamlNodeWriter::write(ryml::csubstr key, const InputType& inputValue)
{
	return YamlNodeWriter(_root, _node.append_child({ ryml::KEY, key }) << inputValue);
}

template <typename InputType, typename Func>
void YamlNodeWriter::write(ryml::csubstr key, const std::vector<InputType>& inputVector, Func callback)
{
	if (inputVector.empty())
		return;
	YamlNodeWriter sequenceWriter(_root, _node.append_child({ ryml::KEY, key }));
	sequenceWriter.setAsSeq();
	for (const InputType& vectorElement : inputVector)
	{
		callback(sequenceWriter, vectorElement);
	}
}

template <typename InputType>
inline void YamlNodeWriter::setValue(const InputType& inputValue)
{
	_node << inputValue;
}

} //namespace YAML end

} //namespace OpenXcom end

// r/w overloads need to be defined in the same namespace the type is defined in
namespace std
{

// Deserializing "" should succeed when output type is std::string
bool read(ryml::ConstNodeRef const& n, std::string* str);

// for backwards compatibility, pairs should be serialized as sequences with 2 elements.
template <class T1, class T2>
bool read(ryml::ConstNodeRef const& n, std::pair<T1, T2>* pair)
{
	n.first_child() >> pair->first;
	n.last_child() >> pair->second;
	return true;
}

template <class T1, class T2>
void write(ryml::NodeRef* n, std::pair<T1, T2> const& pair)
{
	*n |= ryml::SEQ;
	n->append_child() << pair.first;
	n->append_child() << pair.second;
}

} // namespace std end

namespace c4::yml
{
// Serializing bool should output the string version instead of 0 and 1
void write(ryml::NodeRef* n, bool const& val);
}
