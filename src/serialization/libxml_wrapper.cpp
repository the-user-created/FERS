//
// Created by David Young on 10/4/24.
//

#include "libxml_wrapper.h"

bool XmlDocument::validateWithDtd(const std::string& dtdFilename) const
{
	// Load the DTD
	xmlDtdPtr dtd = xmlParseDTD(nullptr, reinterpret_cast<const xmlChar*>(dtdFilename.c_str()));
	if (!dtd) { throw XmlException("Failed to parse DTD: " + dtdFilename); }

	// Create validation context
	xmlValidCtxtPtr validation_ctxt = xmlNewValidCtxt();
	if (!validation_ctxt)
	{
		xmlFreeDtd(dtd);
		throw XmlException("Failed to create validation context.");
	}

	// Validate the document against the DTD
	const bool is_valid = xmlValidateDtd(validation_ctxt, _doc.get(), dtd);
	xmlFreeValidCtxt(validation_ctxt);
	xmlFreeDtd(dtd);

	if (!is_valid) { throw XmlException("XML failed DTD validation."); }

	return true;
}

// XSD Validation
bool XmlDocument::validateWithXsd(const std::string& xsdFilename) const
{
	// Create a schema parser context
	xmlSchemaParserCtxtPtr schema_parser_ctxt = xmlSchemaNewParserCtxt(xsdFilename.c_str());
	if (!schema_parser_ctxt) { throw XmlException("Failed to create schema parser context."); }

	// Parse the schema
	xmlSchemaPtr schema = xmlSchemaParse(schema_parser_ctxt);
	xmlSchemaFreeParserCtxt(schema_parser_ctxt);
	if (!schema) { throw XmlException("Failed to parse schema: " + xsdFilename); }

	// Create a schema validation context
	xmlSchemaValidCtxtPtr schema_valid_ctxt = xmlSchemaNewValidCtxt(schema);
	if (!schema_valid_ctxt)
	{
		xmlSchemaFree(schema);
		throw XmlException("Failed to create schema validation context.");
	}

	// Validate the document against the schema
	const bool is_valid = xmlSchemaValidateDoc(schema_valid_ctxt, _doc.get()) == 0;
	xmlSchemaFreeValidCtxt(schema_valid_ctxt);
	xmlSchemaFree(schema);

	if (!is_valid) { throw XmlException("XML failed XSD validation."); }

	return true;
}
