-- Copyright 2020 Mats Kindahl
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

CREATE FUNCTION appendchildxml(p_node xml, p_xpath text, p_tmp_node xml)
    RETURNS xml
    AS 'MODULE_PATHNAME', 'appendchildxml'
    LANGUAGE C IMMUTABLE STRICT;


CREATE FUNCTION deletexml(p_xml xml, p_xpath_str text)
    RETURNS xml
    AS 'MODULE_PATHNAME', 'deletexml'
    LANGUAGE C IMMUTABLE STRICT;
