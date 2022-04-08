#pragma once

const char* ExpectedFullSwaggerDefinitionHeader = R"(openapi: 3.0.0

info:
  description: This describes the REST API
  version: '1.0.0'
  title:  API

servers:
  - description: Local API
    url: http://{host}:{port}/data/{version}
    variables:
      host:
        default: '192.168.0.123' # This should be set to your hub's local IP
      port:
        enum:
          - '80'
          - '8080'
          - '443'
          - '8443'
        default: '80'
      version: 
        enum:
          - v1
          - v2
        default: v2


paths:
  /:
    get:
      summary: Get the entire DB resource data
      tags:
        - Global
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/Global_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Collection/:
    get:
      summary: Get all entries in the Collection collection
      tags:
        - /Collection
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 type: array
                 items:
                   $ref: '#/components/schemas/SubMessage_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    post:
      summary: Create a new entry in the Collection collection
      tags:
        - /Collection
      requestBody:
        description: Object to add to collection
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/SubMessage_Schema'
      responses:
        '201':
           description: SubMessage created
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized

  /Collection/{id}:
    get:
      summary: Get entry in the Collection collection with given id
      tags:
        - /Collection
      parameters:
        - $ref: '#/components/parameters/idParam'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/SubMessage_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the entry in the Collection collection with given id
      tags:
        - /Collection
      parameters:
        - $ref: '#/components/parameters/idParam'
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/SubMessage_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/SubMessage_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

    delete:
      summary: Delete the entry in the Collection collection with given id
      tags:
        - /Collection
      parameters:
        - $ref: '#/components/parameters/idParam'
      responses:
        '204':
           description: Object deleted
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Single/:
    get:
      summary: Get the Single resource data
      tags:
        - /Single
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the data in the Single resource 
      tags:
        - /Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/ActionTest_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Single/actionOnBool:
    patch:
      summary: Invoke the action actionOnBool() on the Single resource
      tags:
        - /Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: boolean

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Single/actionOnInteger:
    patch:
      summary: Invoke the action actionOnInteger() on the Single resource
      tags:
        - /Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: integer

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Single/actionOnString:
    patch:
      summary: Invoke the action actionOnString() on the Single resource
      tags:
        - /Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: string
              maxLength: 31

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Single/actionOnObject:
    patch:
      summary: Invoke the action actionOnObject() on the Single resource
      tags:
        - /Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              $ref: '#/components/schemas/AllOptionalTypes_Schema'

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /SubDB/:
    get:
      summary: Get the SubDB resource data
      tags:
        - SubDB
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/SubDB_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /SubDB/SubSingle/:
    get:
      summary: Get the SubSingle resource data
      tags:
        - /SubDB/SubSingle
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the data in the SubSingle resource 
      tags:
        - /SubDB/SubSingle
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/ActionTest_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /SubDB/SubSingle/actionOnBool:
    patch:
      summary: Invoke the action actionOnBool() on the SubSingle resource
      tags:
        - /SubDB/SubSingle
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: boolean

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /SubDB/SubSingle/actionOnInteger:
    patch:
      summary: Invoke the action actionOnInteger() on the SubSingle resource
      tags:
        - /SubDB/SubSingle
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: integer

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /SubDB/SubSingle/actionOnString:
    patch:
      summary: Invoke the action actionOnString() on the SubSingle resource
      tags:
        - /SubDB/SubSingle
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              type: string
              maxLength: 31

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /SubDB/SubSingle/actionOnObject:
    patch:
      summary: Invoke the action actionOnObject() on the SubSingle resource
      tags:
        - /SubDB/SubSingle
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema: 
              $ref: '#/components/schemas/AllOptionalTypes_Schema'

      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/ActionTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found
)";

const char* ExpectedFullSwaggerDefinitionComponents = R"(

components:
  
  responses:
    400BadRequest:
      description: Bad Request
      content:
        application/json:
          schema:
            type: object
            properties:
              Error:
                  type: string
  
  parameters :
    idParam:
      in: path
      name: id
      schema:
        type: integer
      required : true
      description : Numeric ID of the resource

  schemas:

    SubMessage_Schema:
      type: object
      properties:
        id:
          readOnly: true
          type: integer
          minimum: 0
        substuff1:
          type: string
          maxLength: 63
        substuff2:
          type: integer
        substuff3:
          type: boolean

    AllOptionalTypes_Schema:
      type: object
      properties:
        id:
          readOnly: true
          type: integer
          minimum: 0
        int8_type:
          type: integer
          minimum: -128
          maximum: 127
        int16_type:
          type: integer
          minimum: -32768
          maximum: 32767
        int32_type:
          type: integer
        int64_type:
          type: integer
        uint8_type:
          type: integer
          minimum: 0
          maximum: 255
        uint16_type:
          type: integer
          minimum: 0
          maximum: 65535
        uint32_type:
          type: integer
          minimum: 0
        uint64_type:
          type: integer
          minimum: 0
        bool_type:
          type: boolean
        string_type:
          type: string
          maxLength: 31
        bytes_type:
          type: string
          maxLength: 48
        submsg_type:
          allOf: [
            { $ref: '#/components/schemas/SubMessage_Schema' }
          ]
        enum_type:
          type: string
          enum:
          - 'Zero'
          - 'First'
          - 'Second'
          - 'Truth'
        bitmask_type:
          type: object
          properties:
            BitZero:
              type: boolean
            BitOne:
              type: boolean
            BitTwo:
              type: boolean
            BitThree:
              type: boolean
            BitFour:
              type: boolean
            BitFive:
              type: boolean
            BitSix:
              type: boolean
            BitSeven:
              type: boolean

    ActionTest_Schema:
      type: object
      properties:
        value1:
          type: string
          maxLength: 63
        value2:
          type: integer
        value3:
          type: boolean

    SubDB_Schema:
      type: object
      properties:
        SubSingle:
          $ref: '#/components/schemas/ActionTest_Schema'

    Global_Schema:
      type: object
      properties:
        Collection:
          type: array
          items:
            $ref: '#/components/schemas/SubMessage_Schema'
        Single:
          $ref: '#/components/schemas/ActionTest_Schema'
        SubDB:
          $ref: '#/components/schemas/SubDB_Schema'
)";

const char *TagsTestSwagger = R"(openapi: 3.0.0

info:
  description: This describes the REST API
  version: '1.0.0'
  title:  API

servers:
  - description: Local API
    url: http://{host}:{port}/data/{version}
    variables:
      host:
        default: '192.168.0.123' # This should be set to your hub's local IP
      port:
        enum:
          - '80'
          - '8080'
          - '443'
          - '8443'
        default: '80'
      version: 
        enum:
          - v1
          - v2
        default: v2


paths:
  /:
    get:
      summary: Get the entire DB resource data
      tags:
        - Global
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/Global_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

  /Single/:
    get:
      summary: Get the Single resource data
      tags:
        - /Single
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/TagsTest_Schema'
        '403':
           description: Not authorized
        '404':
           description: Not found

    patch:
      summary: Update the data in the Single resource 
      tags:
        - /Single
      requestBody:
        description: partial JSON object representing fields you wish to change
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/TagsTest_Schema'
      responses:
        '200':
           description: OK
           content:
             application/json:
               schema:
                 $ref: '#/components/schemas/TagsTest_Schema'
        '400':
           $ref: '#/components/responses/400BadRequest'
        '403':
           description: Not authorized
        '404':
           description: Not found


components:
  
  responses:
    400BadRequest:
      description: Bad Request
      content:
        application/json:
          schema:
            type: object
            properties:
              Error:
                  type: string
  
  parameters :
    idParam:
      in: path
      name: id
      schema:
        type: integer
      required : true
      description : Numeric ID of the resource

  schemas:

    TagsTest_Schema:
      type: object
      properties:
        id:
          readOnly: true
          type: integer
          minimum: 0
        somePassword:
          writeOnly: true
          description: A password field
          type: string
          maxLength: 15
        action:
          writeOnly: true
          type: boolean
        publicStatus:
          readOnly: true
          type: integer
          minimum: 20
          maximum: 255
        publicReadOnlyConfig:
          readOnly: true
          type: integer
          minimum: -128
          maximum: 127
        publicTempConfig:
          type: integer
          minimum: -128
          maximum: 127
        publicConfig:
          type: integer
          minimum: -128
          maximum: 127

    Global_Schema:
      type: object
      properties:
        Single:
          $ref: '#/components/schemas/TagsTest_Schema'
)";