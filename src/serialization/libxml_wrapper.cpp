//
// Created by David Young on 10/4/24.
//

#include "libxml_wrapper.h"

bool XmlDocument::validateWithDtd(const unsigned char* dtdData, const size_t dtdLength) const
{
	// Parse the DTD from memory
	xmlDtdPtr dtd = xmlIOParseDTD(nullptr, xmlParserInputBufferCreateMem(reinterpret_cast<const char*>(dtdData), static_cast<int>(dtdLength), XML_CHAR_ENCODING_UTF8), XML_CHAR_ENCODING_UTF8);
	if (!dtd) { throw XmlException("Failed to parse DTD from memory."); }

	// Create validation context
	xmlValidCtxtPtr validationCtxt = xmlNewValidCtxt();
	if (!validationCtxt)
	{
		xmlFreeDtd(dtd);
		throw XmlException("Failed to create validation context.");
	}

	// Validate the document against the DTD
	const bool is_valid = xmlValidateDtd(validationCtxt, _doc.get(), dtd);
	xmlFreeValidCtxt(validationCtxt);
	xmlFreeDtd(dtd);

	if (!is_valid) { throw XmlException("XML failed DTD validation."); }

	return true;
}

bool XmlDocument::validateWithXsd(const unsigned char* xsdData, const size_t xsdLength) const
{
	// Parse the XSD schema from memory
	xmlSchemaParserCtxtPtr schemaParserCtxt = xmlSchemaNewMemParserCtxt(reinterpret_cast<const char*>(xsdData), static_cast<int>(xsdLength));
	if (!schemaParserCtxt) { throw XmlException("Failed to create schema parser context."); }

	// Parse the schema
	xmlSchemaPtr schema = xmlSchemaParse(schemaParserCtxt);
	xmlSchemaFreeParserCtxt(schemaParserCtxt);
	if (!schema) { throw XmlException("Failed to parse schema from memory."); }

	// Create a schema validation context
	xmlSchemaValidCtxtPtr schemaValidCtxt = xmlSchemaNewValidCtxt(schema);
	if (!schemaValidCtxt)
	{
		xmlSchemaFree(schema);
		throw XmlException("Failed to create schema validation context.");
	}

	// Validate the document against the schema
	const bool is_valid = xmlSchemaValidateDoc(schemaValidCtxt, _doc.get()) == 0;
	xmlSchemaFreeValidCtxt(schemaValidCtxt);
	xmlSchemaFree(schema);

	if (!is_valid) { throw XmlException("XML failed XSD validation."); }

	return true;
}

// Function to merge the contents of an included XML file into the main document
void mergeXmlDocuments(const XmlDocument& mainDoc, const XmlDocument& includedDoc)
{
	const XmlElement main_root = mainDoc.getRootElement();
	const XmlElement included_root = includedDoc.getRootElement();

	// Merge all child elements of the included document's root into the main document's root
	xmlNodePtr child = included_root.getNode()->children;
	while (child)
	{
		if (child->type == XML_ELEMENT_NODE) // Only add elements, ignore text nodes or comments
		{
			xmlNodePtr new_node = xmlCopyNode(child, 1); // Deep copy the child node
			xmlAddChild(main_root.getNode(), new_node); // Add the copied node to the main document
		}
		child = child->next;
	}
}

// Function to remove all include elements from the main document
void removeIncludeElements(const XmlDocument& doc)
{
	const XmlElement root = doc.getRootElement();
	while (true)
	{
		XmlElement include_element = root.childElement("include", 0);
		if (!include_element.isValid()) { break; }

		xmlUnlinkNode(include_element.getNode());
		xmlFreeNode(include_element.getNode());
	}
}
