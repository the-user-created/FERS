//
// Created by David Young on 9/29/24.
//

#ifndef LIBXML_WRAPPER_H
#define LIBXML_WRAPPER_H

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>

class XmlException final : public std::runtime_error
{
public:
	explicit XmlException(const std::string& message) : std::runtime_error(message) {}
};

class XmlElement
{
	xmlNodePtr _node;

public:
	explicit XmlElement(xmlNodePtr node) : _node(node) {}

	// Get the name of the element
	[[nodiscard]] std::string name() const { return reinterpret_cast<const char*>(_node->name); }

	// Get the text content of the element
	[[nodiscard]] std::string getText() const
	{
		if (!_node) { return ""; }
		xmlChar* text = xmlNodeGetContent(_node);
		std::string result = reinterpret_cast<const char*>(text);
		xmlFree(text);
		return result;
	}

	// Set text content of the element
	void setText(const std::string& text) const
	{
		xmlNodeSetContent(_node, reinterpret_cast<const xmlChar*>(text.c_str()));
	}

	// Get an attribute
	static std::string getSafeAttribute(const XmlElement& element, const std::string& name)
	{
		std::string value;
		if (xmlChar* attr = xmlGetProp(element.getNode(), reinterpret_cast<const xmlChar*>(name.c_str())))
		{
			value = reinterpret_cast<const char*>(attr);
			xmlFree(attr); // Always free the attribute after use
		}
		else
		{
			throw XmlException("Attribute not found: " + name);
		}
		return value;
	}

	// Set an attribute
	void setAttribute(const std::string& name, const std::string& value) const
	{
		xmlSetProp(_node, reinterpret_cast<const xmlChar*>(name.c_str()),
		           reinterpret_cast<const xmlChar*>(value.c_str()));
	}

	// Add a child element
	[[nodiscard]] XmlElement linkEndChild(const XmlElement& child) const
	{
		return XmlElement(xmlAddChild(_node, child._node));
	}

	// Add a new child element
	[[nodiscard]] XmlElement addChild(const std::string& name) const
	{
		xmlNodePtr child = xmlNewNode(nullptr, reinterpret_cast<const xmlChar*>(name.c_str()));
		xmlAddChild(_node, child);
		return XmlElement(child);
	}

	// Get the first child element
	[[nodiscard]] XmlElement childElement(const std::string& name, const unsigned index) const
	{
		if (!_node) { return XmlElement(nullptr); }
		unsigned count = 0;
		xmlNodePtr child = _node->children;
		while (child)
		{
			if (child->type == XML_ELEMENT_NODE && (name.empty() || name == reinterpret_cast<const char*>(child->name)))
			{
				if (count == index) { return XmlElement(child); }
				++count;
			}
			child = child->next;
		}
		return XmlElement(nullptr); // Return an invalid XmlElement if the index is out of bounds
	}

	// Check if the element is valid
	[[nodiscard]] bool isValid() const { return _node != nullptr; }

	// Get the underlying xmlNodePtr
	[[nodiscard]] xmlNodePtr getNode() const { return _node; }
};

class XmlDocument
{
	std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> _doc;

public:
	// Constructor to initialize the document
	XmlDocument() : _doc(xmlNewDoc(reinterpret_cast<const xmlChar*>("1.0")), xmlFreeDoc)
	{
		if (!_doc) { throw std::runtime_error("Failed to create XML document"); }
	}

	// Load XML from a file
	bool loadFile(const std::string& filename)
	{
		_doc.reset(xmlReadFile(filename.c_str(), nullptr, 0));
		return _doc != nullptr;
	}

	// Save XML to file
	[[nodiscard]] bool saveFile(const std::string& filename) const
	{
		if (!_doc)
		{
			std::cerr << "Document is null, cannot save." << std::endl;
			return false;
		}

		if (const int result = xmlSaveFormatFileEnc(filename.c_str(), _doc.get(), "UTF-8", 1); result == -1)
		{
			std::cerr << "Failed to save XML to file: " << filename << std::endl;
			return false;
		}

		return true;
	}

	// Set root element
	void setRootElement(const XmlElement& root) const
	{
		if (!_doc) { throw std::runtime_error("Document not created"); }
		xmlDocSetRootElement(_doc.get(), root.getNode());
	}

	// Get root element
	[[nodiscard]] XmlElement getRootElement() const
	{
		if (!_doc) { throw std::runtime_error("Document not loaded"); }
		xmlNodePtr root = xmlDocGetRootElement(_doc.get());
		if (!root) { throw std::runtime_error("Root element not found"); }
		return XmlElement(root);
	}

	// Expose the underlying xmlDocPtr for direct access
	[[nodiscard]] xmlDocPtr getDoc() const { return _doc.get(); }

	static std::string getXmlVersion(const XmlDocument& doc)
	{
		if (xmlDocPtr docPtr = doc.getDoc(); docPtr && docPtr->version)
		{
			return reinterpret_cast<const char*>(docPtr->version);
		}
		return "unknown";
	}
};

class XmlHandle
{
	XmlElement _element;

public:
	explicit XmlHandle(const XmlElement element) : _element(element) {}

	// Return the current element
	[[nodiscard]] XmlElement element() const { return _element; }

	// Return the first child element
	[[nodiscard]] XmlElement firstChildElement(const std::string& name = "") const
	{
		return _element.childElement(name, 0);
	}

	// Navigate to the child element
	[[nodiscard]] XmlHandle childElement(const std::string& name, const unsigned index) const
	{
		return XmlHandle(_element.childElement(name, index));
	}

	[[nodiscard]] bool isValid() const { return _element.isValid(); }
};

#endif //LIBXML_WRAPPER_H
