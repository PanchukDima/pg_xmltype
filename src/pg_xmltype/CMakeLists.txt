# Copyright 2020 Mats Kindahl
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

add_postgresql_extension(
  pg_xmltype
  VERSION 1.2
  SOURCES pg_xmltype.c
  SCRIPTS pg_xmltype--1.2.sql
  REGRESS basic)

target_sources(pg_xmltype
  PRIVATE
    pg_xmltype--1.2.sql
    pg_xmltype.h
)
