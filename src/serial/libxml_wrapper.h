/**
* @file libxml_wrapper.h
* @brief Wrapper for managing XML documents and elements using libxml2.
*
* This header file provides classes and functions to simplify handling XML documents and elements
* using the libxml2 library. It includes basic functionality for manipulating XML nodes, attributes,
* content, and validation using DTD and XSD schemas.
*
* @author David Young
* @date 2024-09-29
*/

#pragma once

#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <libxml/parser.h>

#include "core/logging.h"
#include "libxml/globals.h"
#include "libxml/xmlstring.h"

/**
* @class XmlException
* @brief Exception class for handling XML-related errors.
*
* A specialized exception class that inherits from std::runtime_error. It is used to throw
* exceptions when XML operations fail, such as missing attributes or validation errors.
*/
class XmlException final : public std::runtime_error
{
public:
	/**
	* @brief Constructor for XmlException.
	*
	* Creates an XmlException with the provided error message.
	*
	* @param message The error message associated with the exception.
	*/
	explicit XmlException(const std::string_view message) : std::runtime_error(std::string(message)) {}
};

/**
* @class XmlElement
* @brief Class representing a node in an XML document.
*
* This class encapsulates an XML element, allowing users to access and manipulate
* element names, attributes, content, and children. It uses libxml2 for all operations
* and provides simplified methods to interact with XML nodes.
*/
class XmlElement
{
	xmlNodePtr _node; ///< Pointer to the XML node represented by this object.

public:
	/**
	* @brief Constructor for XmlElement.
	*
	* Initializes an XmlElement with a given XML node.
	*
	* @param node The xmlNode pointer representing the XML element.
	*/
	explicit XmlElement(const xmlNode* node) : _node(const_cast<xmlNode*>(node)) {}

	/**
	* @brief Get the name of the XML element.
	*
	* Returns the name of the current XML node as a string view.
	*
	* @return The name of the XML element.
	*/
	[[nodiscard]] std::string_view name() const noexcept { return reinterpret_cast<const char*>(_node->name); }

	/**
	* @brief Get the text content of the XML element.
	*
	* Retrieves the text content contained within the element.
	*
	* @return The text content as a string.
	*/
	[[nodiscard]] std::string getText() const
	{
		if (!_node) { return ""; }
		xmlChar* text = xmlNodeGetContent(_node);
		std::string result = reinterpret_cast<const char*>(text);
		xmlFree(text);
		return result;
	}

	/**
	* @brief Set the text content of the XML element.
	*
	* Sets the content of the XML node to the provided text.
	*
	* @param text The text to set as the content of the node.
	*/
	void setText(const std::string_view text) const
	{
		xmlNodeSetContent(_node, reinterpret_cast<const xmlChar*>(text.data()));
	}

	/**
	* @brief Get the value of an attribute safely.
	*
	* Retrieves the value of the specified attribute. If the attribute is not found, an XmlException is thrown.
	*
	* @param element The XmlElement to retrieve the attribute from.
	* @param name The name of the attribute.
	* @return The value of the attribute.
	* @throws XmlException if the attribute is not found.
	*/
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

	/**
	* @brief Set an attribute on the XML element.
	*
	* Sets the specified attribute to the provided value.
	*
	* @param name The name of the attribute.
	* @param value The value to set for the attribute.
	*/
	void setAttribute(const std::string_view name, const std::string_view value) const
	{
		xmlSetProp(_node, reinterpret_cast<const xmlChar*>(name.data()),
		           reinterpret_cast<const xmlChar*>(value.data()));
	}

	/**
	* @brief Add a child element to the current node.
	*
	* Creates and adds a new child node with the specified name to the current node.
	*
	* @param name The name of the new child element.
	* @return The newly created XmlElement.
	*/
	[[nodiscard]] XmlElement addChild(const std::string_view name) const noexcept
	{
		const xmlNode* child = xmlNewNode(nullptr, reinterpret_cast<const xmlChar*>(name.data()));
		xmlAddChild(_node, const_cast<xmlNode*>(child));
		return XmlElement(child);
	}

	/**
	* @brief Retrieve a child element by name and index.
	*
	* Searches for a child element by its name and returns the child at the given index. Returns an invalid
	* XmlElement if the child is not found.
	*
	* @param name The name of the child element (optional).
	* @param index The index of the child to retrieve.
	* @return The child element or an invalid XmlElement if not found.
	*/
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

	/**
	* @brief Check if the XML element is valid.
	*
	* Returns whether the element holds a valid node.
	*
	* @return True if the node is valid, otherwise false.
	*/
	[[nodiscard]] bool isValid() const noexcept { return _node != nullptr; }

	/**
	* @brief Get the underlying XML node pointer.
	*
	* Provides access to the raw xmlNodePtr used by libxml2.
	*
	* @return The underlying XML node pointer.
	*/
	[[nodiscard]] xmlNodePtr getNode() const noexcept { return _node; }
};

/**
* @class XmlDocument
* @brief Class for managing XML documents.
*
* This class encapsulates an XML document, providing methods for loading, saving, and manipulating
* the document, as well as validating it against DTD or XSD schemas. It uses libxml2 for document
* parsing and validation.
*/
class XmlDocument
{
	std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> _doc; ///< Pointer to the XML document.

public:
	/**
	* @brief Constructor for XmlDocument.
	*
	* Initializes a new XML document with version "1.0".
	*
	* @throws std::runtime_error if the document creation fails.
	*/
	XmlDocument() : _doc(xmlNewDoc(reinterpret_cast<const xmlChar*>("1.0")), &xmlFreeDoc)
	{
		if (!_doc) { throw XmlException("Failed to create XML document."); }
	}

	/**
	* @brief Load an XML file into the document.
	*
	* Loads the XML content from a file.
	*
	* @param filename The name of the file to load.
	* @return True if the file was successfully loaded, otherwise false.
	*/
	[[nodiscard]] bool loadFile(const std::string_view filename)
	{
		_doc.reset(xmlReadFile(filename.data(), nullptr, 0));
		return _doc != nullptr;
	}

	/**
	* @brief Save the document to a file.
	*
	* Saves the XML document to a specified file.
	*
	* @param filename The name of the file to save to.
	* @return True if the file was successfully saved, otherwise false.
	*/
	[[nodiscard]] bool saveFile(const std::string_view filename) const
	{
		if (!_doc)
		{
			LOG(logging::Level::ERROR, "Document is null; Cannot save to file");
			return false;
		}
		return xmlSaveFormatFileEnc(filename.data(), _doc.get(), "UTF-8", 1) != -1;
	}

	/**
	* @brief Set the root element of the document.
	*
	* Sets the root element of the XML document to the provided element.
	*
	* @param root The root element to set.
	* @throws std::runtime_error if the document is not created.
	*/
	void setRootElement(const XmlElement& root) const
	{
		if (!_doc) { throw std::runtime_error("Document not created"); }
		xmlDocSetRootElement(_doc.get(), root.getNode());
	}

	/**
	* @brief Get the root element of the document.
	*
	* Retrieves the root element of the XML document.
	*
	* @return The root element.
	* @throws std::runtime_error if the document is not loaded or the root element is missing.
	*/
	[[nodiscard]] XmlElement getRootElement() const
	{
		if (!_doc) { throw std::runtime_error("Document not loaded"); }
		const xmlNode* root = xmlDocGetRootElement(_doc.get());
		if (!root) { throw std::runtime_error("Root element not found"); }
		return XmlElement(root);
	}

	/**
	* @brief Validate the document using a DTD.
	*
	* Validates the XML document against the provided DTD data.
	*
	* @param dtdData The DTD data used for validation.
	* @return True if the document is valid according to the DTD.
	* @throws XmlException if the DTD is invalid or the validation fails.
	*/
	[[nodiscard]] bool validateWithDtd(std::span<const unsigned char> dtdData) const;

	/**
	* @brief Validate the document using an XSD schema.
	*
	* Validates the XML document against the provided XSD data.
	*
	* @param xsdData The XSD data used for validation.
	* @return True if the document is valid according to the XSD schema.
	* @throws XmlException if the XSD is invalid or the validation fails.
	*/
	[[nodiscard]] bool validateWithXsd(std::span<const unsigned char> xsdData) const;
};

/**
* @brief Merge two XML documents.
*
* Merges the content of the included document into the main document.
*
* @param mainDoc The main XML document.
* @param includedDoc The XML document to include.
*/
void mergeXmlDocuments(const XmlDocument& mainDoc, const XmlDocument& includedDoc);

/**
* @brief Remove "include" elements from the XML document.
*
* Removes all "include" elements from the given XML document.
*
* @param doc The XML document from which to remove the "include" elements.
*/
void removeIncludeElements(const XmlDocument& doc);
