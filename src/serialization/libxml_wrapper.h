//
// Created by David Young on 9/29/24.
//

#ifndef LIBXML_WRAPPER_H
#define LIBXML_WRAPPER_H

#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/xmlschemas.h>

class XmlException final : public std::runtime_error
{
public:
	explicit XmlException(const std::string_view message) : std::runtime_error(std::string(message)) {}
};

class XmlElement
{
	xmlNodePtr _node;

public:
	explicit XmlElement(xmlNodePtr node) : _node(node) {}

	[[nodiscard]] std::string_view name() const noexcept { return reinterpret_cast<const char*>(_node->name); }

	[[nodiscard]] std::string getText() const
	{
		if (!_node) { return ""; }
		xmlChar* text = xmlNodeGetContent(_node);
		std::string result = reinterpret_cast<const char*>(text);
		xmlFree(text);
		return result;
	}

	void setText(const std::string_view text) const
	{
		xmlNodeSetContent(_node, reinterpret_cast<const xmlChar*>(text.data()));
	}

	static std::string getSafeAttribute(const XmlElement& element, const std::string_view name)
	{
		std::string value;
		if (xmlChar* attr = xmlGetProp(element.getNode(), reinterpret_cast<const xmlChar*>(name.data())))
		{
			value = reinterpret_cast<const char*>(attr);
			xmlFree(attr); // Always free the attribute after use
		}
		else { throw XmlException("Attribute not found: " + std::string(name)); }
		return value;
	}

	void setAttribute(const std::string_view name, const std::string_view value) const
	{
		xmlSetProp(_node, reinterpret_cast<const xmlChar*>(name.data()),
		           reinterpret_cast<const xmlChar*>(value.data()));
	}

	[[nodiscard]] XmlElement addChild(const std::string_view name) const noexcept
	{
		xmlNodePtr child = xmlNewNode(nullptr, reinterpret_cast<const xmlChar*>(name.data()));
		xmlAddChild(_node, child);
		return XmlElement(child);
	}

	[[nodiscard]] XmlElement childElement(const std::string_view name = "", const unsigned index = 0) const noexcept
	{
		if (!_node) { return XmlElement(nullptr); }
		unsigned count = 0;
		for (auto* child = _node->children; child; child = child->next)
		{
			if (child->type == XML_ELEMENT_NODE && (name.empty() || name == reinterpret_cast<const char*>(child->name)))
			{
				if (count == index) { return XmlElement(child); }
				++count;
			}
		}
		return XmlElement(nullptr); // Return an invalid XmlElement if the index is out of bounds
	}

	[[nodiscard]] bool isValid() const noexcept { return _node != nullptr; }

	[[nodiscard]] xmlNodePtr getNode() const noexcept { return _node; }
};

class XmlDocument
{
	std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> _doc;

public:
	XmlDocument() : _doc(xmlNewDoc(reinterpret_cast<const xmlChar*>("1.0")), &xmlFreeDoc)
	{
		if (!_doc) { throw std::runtime_error("Failed to create XML document"); }
	}

	[[nodiscard]] bool loadFile(const std::string_view filename)
	{
		_doc.reset(xmlReadFile(filename.data(), nullptr, 0));
		return _doc != nullptr;
	}

	[[nodiscard]] bool saveFile(const std::string_view filename) const
	{
		if (!_doc)
		{
			std::cerr << "Document is null, cannot save." << std::endl;
			return false;
		}
		return xmlSaveFormatFileEnc(filename.data(), _doc.get(), "UTF-8", 1) != -1;
	}

	void setRootElement(const XmlElement& root) const
	{
		if (!_doc) { throw std::runtime_error("Document not created"); }
		xmlDocSetRootElement(_doc.get(), root.getNode());
	}

	[[nodiscard]] XmlElement getRootElement() const
	{
		if (!_doc) { throw std::runtime_error("Document not loaded"); }
		xmlNodePtr root = xmlDocGetRootElement(_doc.get());
		if (!root) { throw std::runtime_error("Root element not found"); }
		return XmlElement(root);
	}

	[[nodiscard]] bool validateWithDtd(std::span<const unsigned char> dtdData) const;

	[[nodiscard]] bool validateWithXsd(std::span<const unsigned char> xsdData) const;
};

void mergeXmlDocuments(const XmlDocument& mainDoc, const XmlDocument& includedDoc);

void removeIncludeElements(const XmlDocument& doc);

#endif //LIBXML_WRAPPER_H
