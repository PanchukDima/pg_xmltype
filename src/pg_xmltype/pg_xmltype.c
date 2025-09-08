/* Copyright 2020 Mats Kindahl
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

/*
 * This file introduce the quarternion type.
 *
 * It is based on the example un "User-Defined Types" at
 * https://www.postgresql.org/docs/10/xtypes.html
 */

#include "pg_xmltype.h"

#include <postgres.h>
#include <fmgr.h>
#include <utils/xml.h>
#include "utils/builtins.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "libxml/xmlsave.h"

#include "utils/array.h"
#include "utils/elog.h"
#include "catalog/pg_type.h"



int c_appendchildxml(const char* xml_doc, const char* xpath, const char* xml_child_node, char** result_xml)
{
  xmlDocPtr doc = xmlParseDoc((const xmlChar*)xml_doc);
  if (!doc) {
    return -1;
  }

  doc->encoding = xmlStrdup(BAD_CAST "UTF-8");

  xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
  if (!ctx) {
    xmlFreeDoc(doc);
    return -1;
  }

  xmlXPathObjectPtr xpath_result = xmlXPathEvalExpression((const xmlChar*)xpath, ctx);
  if (!xpath_result) {
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
    return -1;
  }

  if (!xpath_result->nodesetval || xpath_result->nodesetval->nodeNr == 0) {
    xmlXPathFreeObject(xpath_result);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
    return -1;
  }

  xmlDocPtr fragment = xmlParseDoc((const xmlChar*)xml_child_node);
  if (fragment) {
    xmlNodePtr fragment_root = xmlDocGetRootElement(fragment);
    if (fragment_root) {
      for (int i = 0; i < xpath_result->nodesetval->nodeNr; i++) {
        xmlNodePtr imported = xmlDocCopyNode(fragment_root, doc, 1);
        if (imported) {
          xmlAddChild(xpath_result->nodesetval->nodeTab[i], imported);
        }
      }
    }
    xmlFreeDoc(fragment);
  }

  xmlChar* output = NULL;
  int size = 0;
  xmlDocDumpMemory(doc, &output, &size);

  if (output) {
    *result_xml = strdup((char*)output);
    xmlFree(output);
  } else {
    *result_xml = strdup(xml_doc);
  }

  xmlXPathFreeObject(xpath_result);
  xmlXPathFreeContext(ctx);
  xmlFreeDoc(doc);

  return 0;
}



int c_deletexml(const char *xml_doc, const char *xpath, char **result_xml)
{
  xmlDocPtr doc = xmlParseDoc((const xmlChar*)xml_doc);
  if (!doc) {
    return -1;
  }

  doc->encoding = xmlStrdup(BAD_CAST "UTF-8");

  xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
  if (!ctx) {
    xmlFreeDoc(doc);
    return -1;
  }

  xmlXPathObjectPtr xpath_result = xmlXPathEvalExpression((const xmlChar*)xpath, ctx);
  if (!xpath_result) {
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
    return -1;
  }

  int nodes_deleted = 0;
  if (xpath_result->nodesetval && xpath_result->nodesetval->nodeNr > 0) {
    xmlNodeSetPtr nodes = xpath_result->nodesetval;

    // Удаляем узлы в обратном порядке (как в Python примере)
    for (int i = nodes->nodeNr - 1; i >= 0; i--) {
      xmlNodePtr node = nodes->nodeTab[i];
      if (node) {
        xmlUnlinkNode(node);
        xmlFreeNode(node);
        nodes_deleted++;
      }
    }
  }

  xmlChar* output = NULL;
  int size = 0;

  xmlDocDumpMemory(doc, &output, &size);

  if (output) {
    *result_xml = strdup((char*)output);
    xmlFree(output);
  } else {
    *result_xml = strdup(xml_doc);
  }

  xmlXPathFreeObject(xpath_result);
  xmlXPathFreeContext(ctx);
  xmlFreeDoc(doc);


  return 0;

}



PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(appendchildxml);

Datum appendchildxml(PG_FUNCTION_ARGS) {
    
    xmltype *node = PG_GETARG_XML_P(0);
    text *xpath_str = PG_GETARG_TEXT_P(1);
    xmltype *child_node = PG_GETARG_XML_P(2);
    char *result_xml = NULL;
    

    // Получаем текстовое представление XML
    text *node_text = xmltotext_with_xmloption(node, XMLOPTION_DOCUMENT);
    text *child_text = xmltotext_with_xmloption(child_node, XMLOPTION_CONTENT);
    
    // Конвертируем в C-строки
    const char *node_cstr = text_to_cstring(node_text);
    const char *child_cstr = text_to_cstring(child_text);
    const char *xpath_cstr = text_to_cstring(xpath_str);


    int res_code = c_appendchildxml(node_cstr, xpath_cstr, child_cstr, &result_xml);

    if (res_code == -1) {

      // Функция должна вернуть -1, но result2 все равно будет содержать копию исходного XML
      if (result_xml) {
        node = xmlparse(cstring_to_text(result_xml), XMLOPTION_CONTENT, false);
      }
    }
    else
    {
      node = xmlparse(cstring_to_text(result_xml), XMLOPTION_CONTENT, false);
    }

    PG_RETURN_XML_P(node);
}


PG_FUNCTION_INFO_V1(deletexml);

Datum deletexml(PG_FUNCTION_ARGS) {
  xmltype *node = PG_GETARG_XML_P(0);
  text *xpath_str = PG_GETARG_TEXT_P(1);
  char *result_xml = NULL;

  text *node_text = xmltotext_with_xmloption(node, XMLOPTION_DOCUMENT);

  const char *node_cstr = text_to_cstring(node_text);
  const char *xpath_cstr = text_to_cstring(xpath_str);


  int res_code = c_deletexml(node_cstr, xpath_cstr, &result_xml);

  if (res_code == -1) {

    // Функция должна вернуть -1, но result2 все равно будет содержать копию исходного XML
    if (result_xml) {
      node = xmlparse(cstring_to_text(result_xml), XMLOPTION_CONTENT, false);
    }
  }
  else
  {
    node = xmlparse(cstring_to_text(result_xml), XMLOPTION_CONTENT, false);
  }

  PG_RETURN_XML_P(node);
}

PG_FUNCTION_INFO_V1(getarrayxml);

Datum
getarrayxml(PG_FUNCTION_ARGS)
{
  xmltype    *xml_data;
  text       *xpath_text;
  char       *xpath_str = "//*"; // default value
  xmlDocPtr   doc = NULL;
  xmlXPathContextPtr context = NULL;
  xmlXPathObjectPtr result = NULL;
  xmlNodeSetPtr nodes;
  Datum      *dvalues = NULL;
  bool       *dnulls = NULL;
  ArrayType  *array;
  int         i;
  int         count;
  MemoryContext oldcontext;

  if (PG_ARGISNULL(0))
    PG_RETURN_NULL();

  xml_data = PG_GETARG_XML_P(0);

  if (!PG_ARGISNULL(1))
  {
    xpath_text = PG_GETARG_TEXT_P(1);
    xpath_str = text_to_cstring(xpath_text);
  }

  /* Switch to multi-call memory context for large allocations */
  oldcontext = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);

  /* Parse XML document */
  doc = xmlParseMemory(VARDATA(xml_data), VARSIZE(xml_data) - VARHDRSZ);
  if (doc == NULL)
  {
    MemoryContextSwitchTo(oldcontext);
    ereport(ERROR,
            (errcode(ERRCODE_INVALID_XML_DOCUMENT),
             errmsg("invalid XML document")));
  }

  /* Create XPath context */
  context = xmlXPathNewContext(doc);
  if (context == NULL)
  {
    xmlFreeDoc(doc);
    MemoryContextSwitchTo(oldcontext);
    ereport(ERROR,
            (errcode(ERRCODE_OUT_OF_MEMORY),
             errmsg("could not create XPath context")));
  }

  /* Evaluate XPath expression */
  result = xmlXPathEvalExpression((xmlChar *)xpath_str, context);
  if (result == NULL)
  {
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
    MemoryContextSwitchTo(oldcontext);
    ereport(ERROR,
            (errcode(ERRCODE_INVALID_XML_DOCUMENT),
             errmsg("XPath evaluation failed")));
  }

  if (result->type != XPATH_NODESET)
  {
    xmlXPathFreeObject(result);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
    MemoryContextSwitchTo(oldcontext);
    ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
             errmsg("XPath expression must return a node set")));
  }

  nodes = result->nodesetval;
  count = (nodes != NULL) ? nodes->nodeNr : 0;

  /* Allocate memory for array elements */
  dvalues = (Datum *) palloc(count * sizeof(Datum));
  dnulls = (bool *) palloc(count * sizeof(bool));
  memset(dnulls, false, count * sizeof(bool));

  /* Convert nodes to XML datums */
  for (i = 0; i < count; i++)
  {
    xmlBufferPtr buf = NULL;
    xmlNodePtr node = nodes->nodeTab[i];
    xmlChar *node_str;
    int node_len;
    char *c_node_str;

    buf = xmlBufferCreate();
    if (buf == NULL)
    {
      ereport(WARNING,
              (errcode(ERRCODE_OUT_OF_MEMORY),
               errmsg("failed to create XML buffer")));
      dnulls[i] = true;
      continue;
    }

    xmlNodeDump(buf, doc, node, 0, 1);

    node_str = xmlBufferContent(buf);
    node_len = xmlBufferLength(buf);

    if (node_str != NULL && node_len > 0)
    {
      /* Convert xmlChar* to char* for xml_in */
      c_node_str = (char *)node_str;
      xmltype *xml_node = xmlparse(cstring_to_text(c_node_str), XMLOPTION_CONTENT, false);
      dvalues[i] = PointerGetDatum(xml_node);
    }
    else
    {
      dnulls[i] = true;
    }

    if (buf != NULL)
      xmlBufferFree(buf);
  }

  /* Create result array */
  if (count > 0)
  {
    int dims[1];
    int lbs[1];

    dims[0] = count;
    lbs[0] = 1;

    array = construct_md_array(dvalues, dnulls, 1, dims, lbs,
                               XMLOID, -1, false, 'd');
  }
  else
  {
    array = construct_empty_array(XMLOID);
  }

  /* Cleanup */
  if (result != NULL)
    xmlXPathFreeObject(result);
  if (context != NULL)
    xmlXPathFreeContext(context);
  if (doc != NULL)
    xmlFreeDoc(doc);

  if (dvalues != NULL)
    pfree(dvalues);
  if (dnulls != NULL)
    pfree(dnulls);

  if (xpath_str != "//*")
    pfree(xpath_str);

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_ARRAYTYPE_P(array);
}

/*Datum getarrayxml(PG_FUNCTION_ARGS) {

  elog(NOTICE, "START");
  xmltype *node = PG_GETARG_XML_P(0);
  text *xpath_str = PG_GETARG_TEXT_P(1);
  ArrayType  *arr;
  Datum      *dvalues;
  bool       *dnulls;
  int         ndims;
  int         dims[1];
  int         lbs[1];
  int         nelems;
  int         i;
  elog(NOTICE, "Work");
  PG_TRY();
  {
    dims[0] = 1;
    lbs[0] = 1;

    dvalues = (Datum *) palloc(sizeof(Datum));
    dnulls = (bool *) palloc(sizeof(bool));

    dvalues[0] = node;
    dnulls[0] = false;

    arr = construct_md_array(dvalues, dnulls, 1, dims, lbs,
                                XMLOID, -1, false, 'd');
    PG_RETURN_ARRAYTYPE_P(arr);
  }
  PG_CATCH();
  {
    elog(NOTICE, "Error build array");
    arr = construct_empty_array(XMLOID);
    PG_RETURN_ARRAYTYPE_P(arr);
  }
  PG_END_TRY();
}*/
