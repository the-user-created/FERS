//
// Created by David Young on 10/4/24.
//

#include "libxml_wrapper.h"

bool XmlDocument::validateWithDtd(const std::span<const unsigned char> dtdData) const
{
	// Parse the DTD from memory using span
	xmlDtdPtr dtd = xmlIOParseDTD(nullptr,
	                              xmlParserInputBufferCreateMem(reinterpret_cast<const char*>(dtdData.data()),
	                                                            static_cast<int>(dtdData.size()),
	                                                            XML_CHAR_ENCODING_UTF8),
	                              XML_CHAR_ENCODING_UTF8);
	if (!dtd) { throw XmlException("Failed to parse DTD from memory."); }

	// Create validation context
	const std::unique_ptr<xmlValidCtxt, decltype(&xmlFreeValidCtxt)> validation_ctxt(
		xmlNewValidCtxt(), xmlFreeValidCtxt);
	if (!validation_ctxt)
	{
		xmlFreeDtd(dtd);
		throw XmlException("Failed to create validation context.");
	}

	// Validate the document against the DTD
	const bool is_valid = xmlValidateDtd(validation_ctxt.get(), _doc.get(), dtd);
	xmlFreeDtd(dtd);

	if (!is_valid) { throw XmlException("XML failed DTD validation."); }

	return true;
}

bool XmlDocument::validateWithXsd(const std::span<const unsigned char> xsdData) const
{
	// Parse the XSD schema from memory using span
	const std::unique_ptr<xmlSchemaParserCtxt, decltype(&xmlSchemaFreeParserCtxt)> schema_parser_ctxt(
		xmlSchemaNewMemParserCtxt(reinterpret_cast<const char*>(xsdData.data()), static_cast<int>(xsdData.size())),
		xmlSchemaFreeParserCtxt
	);
	if (!schema_parser_ctxt) { throw XmlException("Failed to create schema parser context."); }

	// Parse the schema
	const std::unique_ptr<xmlSchema, decltype(&xmlSchemaFree)> schema(xmlSchemaParse(schema_parser_ctxt.get()),
	                                                                  xmlSchemaFree);
	if (!schema) { throw XmlException("Failed to parse schema from memory."); }

	// Create a schema validation context
	const std::unique_ptr<xmlSchemaValidCtxt, decltype(&xmlSchemaFreeValidCtxt)> schema_valid_ctxt(
		xmlSchemaNewValidCtxt(schema.get()), xmlSchemaFreeValidCtxt);
	if (!schema_valid_ctxt) { throw XmlException("Failed to create schema validation context."); }

	// Validate the document against the schema

	if (const bool is_valid = xmlSchemaValidateDoc(schema_valid_ctxt.get(), _doc.get()) == 0; !is_valid)
	{
		throw XmlException("XML failed XSD validation.");
	}

	return true;
}

void mergeXmlDocuments(const XmlDocument& mainDoc, const XmlDocument& includedDoc)
{
	const XmlElement main_root = mainDoc.getRootElement();
	const XmlElement included_root = includedDoc.getRootElement();

	for (xmlNodePtr child = included_root.getNode()->children; child; child = child->next)
	{
		if (child->type == XML_ELEMENT_NODE) // Only add elements, ignore text nodes or comments
		{
			xmlNodePtr new_node = xmlCopyNode(child, 1); // Deep copy the child node
			xmlAddChild(main_root.getNode(), new_node); // Add the copied node to the main document
		}
	}
}

void removeIncludeElements(const XmlDocument& doc)
{
	const XmlElement root = doc.getRootElement();

	// Use an infinite loop with a break condition to remove all "include" elements
	while (true)
	{
		if (XmlElement include_element = root.childElement("include", 0); include_element.isValid())
		{
			xmlUnlinkNode(include_element.getNode());
			xmlFreeNode(include_element.getNode());
		}
		else { break; }
	}
}
