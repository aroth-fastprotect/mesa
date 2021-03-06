/**************************************************************************

Copyright 2002 VMware, Inc.
Copyright 2011 Dave Airlie (ARB_vertex_type_2_10_10_10_rev support)
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "util/format_r11g11b10f.h"
#include "main/varray.h"
#include "vbo_util.h"


/* ATTR */
#define ATTRI( A, N, V0, V1, V2, V3 ) \
    ATTR_UNION(A, N, GL_INT, fi_type, INT_AS_UNION(V0), INT_AS_UNION(V1), \
        INT_AS_UNION(V2), INT_AS_UNION(V3))
#define ATTRUI( A, N, V0, V1, V2, V3 ) \
    ATTR_UNION(A, N, GL_UNSIGNED_INT, fi_type, UINT_AS_UNION(V0), UINT_AS_UNION(V1), \
        UINT_AS_UNION(V2), UINT_AS_UNION(V3))
#define ATTRF( A, N, V0, V1, V2, V3 ) \
    ATTR_UNION(A, N, GL_FLOAT, fi_type, FLOAT_AS_UNION(V0), FLOAT_AS_UNION(V1),\
        FLOAT_AS_UNION(V2), FLOAT_AS_UNION(V3))
#define ATTRD( A, N, V0, V1, V2, V3 ) \
    ATTR_UNION(A, N, GL_DOUBLE, double, V0, V1, V2, V3)
#define ATTRUI64( A, N, V0, V1, V2, V3 ) \
    ATTR_UNION(A, N, GL_UNSIGNED_INT64_ARB, uint64_t, V0, V1, V2, V3)


/* float */
#define ATTR1FV( A, V ) ATTRF( A, 1, (V)[0], 0, 0, 1 )
#define ATTR2FV( A, V ) ATTRF( A, 2, (V)[0], (V)[1], 0, 1 )
#define ATTR3FV( A, V ) ATTRF( A, 3, (V)[0], (V)[1], (V)[2], 1 )
#define ATTR4FV( A, V ) ATTRF( A, 4, (V)[0], (V)[1], (V)[2], (V)[3] )

#define ATTR1F( A, X )          ATTRF( A, 1, X, 0, 0, 1 )
#define ATTR2F( A, X, Y )       ATTRF( A, 2, X, Y, 0, 1 )
#define ATTR3F( A, X, Y, Z )    ATTRF( A, 3, X, Y, Z, 1 )
#define ATTR4F( A, X, Y, Z, W ) ATTRF( A, 4, X, Y, Z, W )


/* int */
#define ATTR2IV( A, V ) ATTRI( A, 2, (V)[0], (V)[1], 0, 1 )
#define ATTR3IV( A, V ) ATTRI( A, 3, (V)[0], (V)[1], (V)[2], 1 )
#define ATTR4IV( A, V ) ATTRI( A, 4, (V)[0], (V)[1], (V)[2], (V)[3] )

#define ATTR1I( A, X )          ATTRI( A, 1, X, 0, 0, 1 )
#define ATTR2I( A, X, Y )       ATTRI( A, 2, X, Y, 0, 1 )
#define ATTR3I( A, X, Y, Z )    ATTRI( A, 3, X, Y, Z, 1 )
#define ATTR4I( A, X, Y, Z, W ) ATTRI( A, 4, X, Y, Z, W )


/* uint */
#define ATTR2UIV( A, V ) ATTRUI( A, 2, (V)[0], (V)[1], 0, 1 )
#define ATTR3UIV( A, V ) ATTRUI( A, 3, (V)[0], (V)[1], (V)[2], 1 )
#define ATTR4UIV( A, V ) ATTRUI( A, 4, (V)[0], (V)[1], (V)[2], (V)[3] )

#define ATTR1UI( A, X )          ATTRUI( A, 1, X, 0, 0, 1 )
#define ATTR2UI( A, X, Y )       ATTRUI( A, 2, X, Y, 0, 1 )
#define ATTR3UI( A, X, Y, Z )    ATTRUI( A, 3, X, Y, Z, 1 )
#define ATTR4UI( A, X, Y, Z, W ) ATTRUI( A, 4, X, Y, Z, W )

#define MAT_ATTR( A, N, V ) ATTRF( A, N, (V)[0], (V)[1], (V)[2], (V)[3] )

#define ATTRUI10_1( A, UI ) ATTRF( A, 1, (UI) & 0x3ff, 0, 0, 1 )
#define ATTRUI10_2( A, UI ) ATTRF( A, 2, (UI) & 0x3ff, ((UI) >> 10) & 0x3ff, 0, 1 )
#define ATTRUI10_3( A, UI ) ATTRF( A, 3, (UI) & 0x3ff, ((UI) >> 10) & 0x3ff, ((UI) >> 20) & 0x3ff, 1 )
#define ATTRUI10_4( A, UI ) ATTRF( A, 4, (UI) & 0x3ff, ((UI) >> 10) & 0x3ff, ((UI) >> 20) & 0x3ff, ((UI) >> 30) & 0x3 )

#define ATTRUI10N_1( A, UI ) ATTRF( A, 1, conv_ui10_to_norm_float((UI) & 0x3ff), 0, 0, 1 )
#define ATTRUI10N_2( A, UI ) ATTRF( A, 2, \
				   conv_ui10_to_norm_float((UI) & 0x3ff), \
				   conv_ui10_to_norm_float(((UI) >> 10) & 0x3ff), 0, 1 )
#define ATTRUI10N_3( A, UI ) ATTRF( A, 3, \
				   conv_ui10_to_norm_float((UI) & 0x3ff), \
				   conv_ui10_to_norm_float(((UI) >> 10) & 0x3ff), \
				   conv_ui10_to_norm_float(((UI) >> 20) & 0x3ff), 1 )
#define ATTRUI10N_4( A, UI ) ATTRF( A, 4, \
				   conv_ui10_to_norm_float((UI) & 0x3ff), \
				   conv_ui10_to_norm_float(((UI) >> 10) & 0x3ff), \
				   conv_ui10_to_norm_float(((UI) >> 20) & 0x3ff), \
				   conv_ui2_to_norm_float(((UI) >> 30) & 0x3) )

#define ATTRI10_1( A, I10 ) ATTRF( A, 1, conv_i10_to_i((I10) & 0x3ff), 0, 0, 1 )
#define ATTRI10_2( A, I10 ) ATTRF( A, 2, \
				conv_i10_to_i((I10) & 0x3ff),		\
				conv_i10_to_i(((I10) >> 10) & 0x3ff), 0, 1 )
#define ATTRI10_3( A, I10 ) ATTRF( A, 3, \
				conv_i10_to_i((I10) & 0x3ff),	    \
				conv_i10_to_i(((I10) >> 10) & 0x3ff), \
				conv_i10_to_i(((I10) >> 20) & 0x3ff), 1 )
#define ATTRI10_4( A, I10 ) ATTRF( A, 4, \
				conv_i10_to_i((I10) & 0x3ff),		\
				conv_i10_to_i(((I10) >> 10) & 0x3ff), \
				conv_i10_to_i(((I10) >> 20) & 0x3ff), \
				conv_i2_to_i(((I10) >> 30) & 0x3))


#define ATTRI10N_1(ctx, A, I10) ATTRF(A, 1, conv_i10_to_norm_float(ctx, (I10) & 0x3ff), 0, 0, 1 )
#define ATTRI10N_2(ctx, A, I10) ATTRF(A, 2, \
				conv_i10_to_norm_float(ctx, (I10) & 0x3ff),		\
				conv_i10_to_norm_float(ctx, ((I10) >> 10) & 0x3ff), 0, 1 )
#define ATTRI10N_3(ctx, A, I10) ATTRF(A, 3, \
				conv_i10_to_norm_float(ctx, (I10) & 0x3ff),	    \
				conv_i10_to_norm_float(ctx, ((I10) >> 10) & 0x3ff), \
				conv_i10_to_norm_float(ctx, ((I10) >> 20) & 0x3ff), 1 )
#define ATTRI10N_4(ctx, A, I10) ATTRF(A, 4, \
				conv_i10_to_norm_float(ctx, (I10) & 0x3ff),		\
				conv_i10_to_norm_float(ctx, ((I10) >> 10) & 0x3ff), \
				conv_i10_to_norm_float(ctx, ((I10) >> 20) & 0x3ff), \
				conv_i2_to_norm_float(ctx, ((I10) >> 30) & 0x3))

#define ATTR_UI(ctx, val, type, normalized, attr, arg) do {	\
   if ((type) == GL_UNSIGNED_INT_2_10_10_10_REV) {		\
      if (normalized) {						\
	 ATTRUI10N_##val((attr), (arg));			\
      } else {							\
	 ATTRUI10_##val((attr), (arg));				\
      }								\
   } else if ((type) == GL_INT_2_10_10_10_REV) {		\
      if (normalized) {						\
	 ATTRI10N_##val(ctx, (attr), (arg));			\
      } else {							\
	 ATTRI10_##val((attr), (arg));				\
      }								\
   } else if ((type) == GL_UNSIGNED_INT_10F_11F_11F_REV) {	\
      float res[4];						\
      res[3] = 1;                                               \
      r11g11b10f_to_float3((arg), res);				\
      ATTR##val##FV((attr), res);				\
   } else							\
      ERROR(GL_INVALID_VALUE);					\
   } while(0)

#define ATTR_UI_INDEX(ctx, val, type, normalized, index, arg) do {	\
      if ((index) == 0 && _mesa_attr_zero_aliases_vertex(ctx)) {	\
	 ATTR_UI(ctx, val, (type), normalized, 0, (arg));		\
      } else if ((index) < MAX_VERTEX_GENERIC_ATTRIBS) {		\
	 ATTR_UI(ctx, val, (type), normalized, VBO_ATTRIB_GENERIC0 + (index), (arg)); \
      } else								\
	 ERROR(GL_INVALID_VALUE);					\
   } while(0)


/* Doubles */
#define ATTR1DV( A, V ) ATTRD( A, 1, (V)[0], 0, 0, 1 )
#define ATTR2DV( A, V ) ATTRD( A, 2, (V)[0], (V)[1], 0, 1 )
#define ATTR3DV( A, V ) ATTRD( A, 3, (V)[0], (V)[1], (V)[2], 1 )
#define ATTR4DV( A, V ) ATTRD( A, 4, (V)[0], (V)[1], (V)[2], (V)[3] )

#define ATTR1D( A, X )          ATTRD( A, 1, X, 0, 0, 1 )
#define ATTR2D( A, X, Y )       ATTRD( A, 2, X, Y, 0, 1 )
#define ATTR3D( A, X, Y, Z )    ATTRD( A, 3, X, Y, Z, 1 )
#define ATTR4D( A, X, Y, Z, W ) ATTRD( A, 4, X, Y, Z, W )

#define ATTR1UIV64( A, V ) ATTRUI64( A, 1, (V)[0], 0, 0, 0 )
#define ATTR1UI64( A, X )  ATTRUI64( A, 1, X, 0, 0, 0 )


static void GLAPIENTRY
TAG(Vertex2f)(GLfloat x, GLfloat y)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR2F(VBO_ATTRIB_POS, x, y);
}

static void GLAPIENTRY
TAG(Vertex2fv)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR2FV(VBO_ATTRIB_POS, v);
}

static void GLAPIENTRY
TAG(Vertex3f)(GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR3F(VBO_ATTRIB_POS, x, y, z);
}

static void GLAPIENTRY
TAG(Vertex3fv)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR3FV(VBO_ATTRIB_POS, v);
}

static void GLAPIENTRY
TAG(Vertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR4F(VBO_ATTRIB_POS, x, y, z, w);
}

static void GLAPIENTRY
TAG(Vertex4fv)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR4FV(VBO_ATTRIB_POS, v);
}



static void GLAPIENTRY
TAG(TexCoord1f)(GLfloat x)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR1F(VBO_ATTRIB_TEX0, x);
}

static void GLAPIENTRY
TAG(TexCoord1fv)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR1FV(VBO_ATTRIB_TEX0, v);
}

static void GLAPIENTRY
TAG(TexCoord2f)(GLfloat x, GLfloat y)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR2F(VBO_ATTRIB_TEX0, x, y);
}

static void GLAPIENTRY
TAG(TexCoord2fv)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR2FV(VBO_ATTRIB_TEX0, v);
}

static void GLAPIENTRY
TAG(TexCoord3f)(GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR3F(VBO_ATTRIB_TEX0, x, y, z);
}

static void GLAPIENTRY
TAG(TexCoord3fv)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR3FV(VBO_ATTRIB_TEX0, v);
}

static void GLAPIENTRY
TAG(TexCoord4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR4F(VBO_ATTRIB_TEX0, x, y, z, w);
}

static void GLAPIENTRY
TAG(TexCoord4fv)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR4FV(VBO_ATTRIB_TEX0, v);
}



static void GLAPIENTRY
TAG(Normal3f)(GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR3F(VBO_ATTRIB_NORMAL, x, y, z);
}

static void GLAPIENTRY
TAG(Normal3fv)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR3FV(VBO_ATTRIB_NORMAL, v);
}



static void GLAPIENTRY
TAG(FogCoordfEXT)(GLfloat x)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR1F(VBO_ATTRIB_FOG, x);
}



static void GLAPIENTRY
TAG(FogCoordfvEXT)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR1FV(VBO_ATTRIB_FOG, v);
}

static void GLAPIENTRY
TAG(Color3f)(GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR3F(VBO_ATTRIB_COLOR0, x, y, z);
}

static void GLAPIENTRY
TAG(Color3fv)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR3FV(VBO_ATTRIB_COLOR0, v);
}

static void GLAPIENTRY
TAG(Color4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR4F(VBO_ATTRIB_COLOR0, x, y, z, w);
}

static void GLAPIENTRY
TAG(Color4fv)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR4FV(VBO_ATTRIB_COLOR0, v);
}



static void GLAPIENTRY
TAG(SecondaryColor3fEXT)(GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR3F(VBO_ATTRIB_COLOR1, x, y, z);
}

static void GLAPIENTRY
TAG(SecondaryColor3fvEXT)(const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR3FV(VBO_ATTRIB_COLOR1, v);
}



static void GLAPIENTRY
TAG(EdgeFlag)(GLboolean b)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR1F(VBO_ATTRIB_EDGEFLAG, (GLfloat) b);
}



static void GLAPIENTRY
TAG(Indexf)(GLfloat f)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR1F(VBO_ATTRIB_COLOR_INDEX, f);
}

static void GLAPIENTRY
TAG(Indexfv)(const GLfloat * f)
{
   GET_CURRENT_CONTEXT(ctx);
   ATTR1FV(VBO_ATTRIB_COLOR_INDEX, f);
}



static void GLAPIENTRY
TAG(MultiTexCoord1f)(GLenum target, GLfloat x)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR1F(attr, x);
}

static void GLAPIENTRY
TAG(MultiTexCoord1fv)(GLenum target, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR1FV(attr, v);
}

static void GLAPIENTRY
TAG(MultiTexCoord2f)(GLenum target, GLfloat x, GLfloat y)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR2F(attr, x, y);
}

static void GLAPIENTRY
TAG(MultiTexCoord2fv)(GLenum target, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR2FV(attr, v);
}

static void GLAPIENTRY
TAG(MultiTexCoord3f)(GLenum target, GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR3F(attr, x, y, z);
}

static void GLAPIENTRY
TAG(MultiTexCoord3fv)(GLenum target, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR3FV(attr, v);
}

static void GLAPIENTRY
TAG(MultiTexCoord4f)(GLenum target, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR4F(attr, x, y, z, w);
}

static void GLAPIENTRY
TAG(MultiTexCoord4fv)(GLenum target, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ATTR4FV(attr, v);
}


static void GLAPIENTRY
TAG(VertexAttrib1fARB)(GLuint index, GLfloat x)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR1F(0, x);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR1F(VBO_ATTRIB_GENERIC0 + index, x);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttrib1fvARB)(GLuint index, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR1FV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR1FV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttrib2fARB)(GLuint index, GLfloat x, GLfloat y)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR2F(0, x, y);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR2F(VBO_ATTRIB_GENERIC0 + index, x, y);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttrib2fvARB)(GLuint index, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR2FV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR2FV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttrib3fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR3F(0, x, y, z);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR3F(VBO_ATTRIB_GENERIC0 + index, x, y, z);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttrib3fvARB)(GLuint index, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR3FV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR3FV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttrib4fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR4F(0, x, y, z, w);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR4F(VBO_ATTRIB_GENERIC0 + index, x, y, z, w);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttrib4fvARB)(GLuint index, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR4FV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR4FV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}



/* Integer-valued generic attributes.
 * XXX: the integers just get converted to floats at this time
 */
static void GLAPIENTRY
TAG(VertexAttribI1i)(GLuint index, GLint x)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR1I(0, x);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR1I(VBO_ATTRIB_GENERIC0 + index, x);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI2i)(GLuint index, GLint x, GLint y)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR2I(0, x, y);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR2I(VBO_ATTRIB_GENERIC0 + index, x, y);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI3i)(GLuint index, GLint x, GLint y, GLint z)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR3I(0, x, y, z);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR3I(VBO_ATTRIB_GENERIC0 + index, x, y, z);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI4i)(GLuint index, GLint x, GLint y, GLint z, GLint w)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR4I(0, x, y, z, w);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR4I(VBO_ATTRIB_GENERIC0 + index, x, y, z, w);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI2iv)(GLuint index, const GLint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR2IV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR2IV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI3iv)(GLuint index, const GLint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR3IV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR3IV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI4iv)(GLuint index, const GLint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR4IV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR4IV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}



/* Unsigned integer-valued generic attributes.
 * XXX: the integers just get converted to floats at this time
 */
static void GLAPIENTRY
TAG(VertexAttribI1ui)(GLuint index, GLuint x)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR1UI(0, x);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR1UI(VBO_ATTRIB_GENERIC0 + index, x);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI2ui)(GLuint index, GLuint x, GLuint y)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR2UI(0, x, y);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR2UI(VBO_ATTRIB_GENERIC0 + index, x, y);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI3ui)(GLuint index, GLuint x, GLuint y, GLuint z)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR3UI(0, x, y, z);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR3UI(VBO_ATTRIB_GENERIC0 + index, x, y, z);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI4ui)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR4UI(0, x, y, z, w);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR4UI(VBO_ATTRIB_GENERIC0 + index, x, y, z, w);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI2uiv)(GLuint index, const GLuint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR2UIV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR2UIV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI3uiv)(GLuint index, const GLuint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR3UIV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR3UIV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribI4uiv)(GLuint index, const GLuint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR4UIV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR4UIV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}



/* These entrypoints are no longer used for NV_vertex_program but they are
 * used by the display list and other code specifically because of their
 * property of aliasing with the legacy Vertex, TexCoord, Normal, etc
 * attributes.  (See vbo_save_loopback.c)
 */
static void GLAPIENTRY
TAG(VertexAttrib1fNV)(GLuint index, GLfloat x)
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < VBO_ATTRIB_MAX)
      ATTR1F(index, x);
}

static void GLAPIENTRY
TAG(VertexAttrib1fvNV)(GLuint index, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < VBO_ATTRIB_MAX)
      ATTR1FV(index, v);
}

static void GLAPIENTRY
TAG(VertexAttrib2fNV)(GLuint index, GLfloat x, GLfloat y)
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < VBO_ATTRIB_MAX)
      ATTR2F(index, x, y);
}

static void GLAPIENTRY
TAG(VertexAttrib2fvNV)(GLuint index, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < VBO_ATTRIB_MAX)
      ATTR2FV(index, v);
}

static void GLAPIENTRY
TAG(VertexAttrib3fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < VBO_ATTRIB_MAX)
      ATTR3F(index, x, y, z);
}

static void GLAPIENTRY
TAG(VertexAttrib3fvNV)(GLuint index,
 const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < VBO_ATTRIB_MAX)
      ATTR3FV(index, v);
}

static void GLAPIENTRY
TAG(VertexAttrib4fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < VBO_ATTRIB_MAX)
      ATTR4F(index, x, y, z, w);
}

static void GLAPIENTRY
TAG(VertexAttrib4fvNV)(GLuint index, const GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < VBO_ATTRIB_MAX)
      ATTR4FV(index, v);
}

static void GLAPIENTRY
TAG(VertexP2ui)(GLenum type, GLuint value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glVertexP2ui");
   ATTR_UI(ctx, 2, type, 0, VBO_ATTRIB_POS, value);
}

static void GLAPIENTRY
TAG(VertexP2uiv)(GLenum type, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glVertexP2uiv");
   ATTR_UI(ctx, 2, type, 0, VBO_ATTRIB_POS, value[0]);
}

static void GLAPIENTRY
TAG(VertexP3ui)(GLenum type, GLuint value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glVertexP3ui");
   ATTR_UI(ctx, 3, type, 0, VBO_ATTRIB_POS, value);
}

static void GLAPIENTRY
TAG(VertexP3uiv)(GLenum type, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glVertexP3uiv");
   ATTR_UI(ctx, 3, type, 0, VBO_ATTRIB_POS, value[0]);
}

static void GLAPIENTRY
TAG(VertexP4ui)(GLenum type, GLuint value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glVertexP4ui");
   ATTR_UI(ctx, 4, type, 0, VBO_ATTRIB_POS, value);
}

static void GLAPIENTRY
TAG(VertexP4uiv)(GLenum type, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glVertexP4uiv");
   ATTR_UI(ctx, 4, type, 0, VBO_ATTRIB_POS, value[0]);
}

static void GLAPIENTRY
TAG(TexCoordP1ui)(GLenum type, GLuint coords)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glTexCoordP1ui");
   ATTR_UI(ctx, 1, type, 0, VBO_ATTRIB_TEX0, coords);
}

static void GLAPIENTRY
TAG(TexCoordP1uiv)(GLenum type, const GLuint *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glTexCoordP1uiv");
   ATTR_UI(ctx, 1, type, 0, VBO_ATTRIB_TEX0, coords[0]);
}

static void GLAPIENTRY
TAG(TexCoordP2ui)(GLenum type, GLuint coords)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glTexCoordP2ui");
   ATTR_UI(ctx, 2, type, 0, VBO_ATTRIB_TEX0, coords);
}

static void GLAPIENTRY
TAG(TexCoordP2uiv)(GLenum type, const GLuint *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glTexCoordP2uiv");
   ATTR_UI(ctx, 2, type, 0, VBO_ATTRIB_TEX0, coords[0]);
}

static void GLAPIENTRY
TAG(TexCoordP3ui)(GLenum type, GLuint coords)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glTexCoordP3ui");
   ATTR_UI(ctx, 3, type, 0, VBO_ATTRIB_TEX0, coords);
}

static void GLAPIENTRY
TAG(TexCoordP3uiv)(GLenum type, const GLuint *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glTexCoordP3uiv");
   ATTR_UI(ctx, 3, type, 0, VBO_ATTRIB_TEX0, coords[0]);
}

static void GLAPIENTRY
TAG(TexCoordP4ui)(GLenum type, GLuint coords)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glTexCoordP4ui");
   ATTR_UI(ctx, 4, type, 0, VBO_ATTRIB_TEX0, coords);
}

static void GLAPIENTRY
TAG(TexCoordP4uiv)(GLenum type, const GLuint *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glTexCoordP4uiv");
   ATTR_UI(ctx, 4, type, 0, VBO_ATTRIB_TEX0, coords[0]);
}

static void GLAPIENTRY
TAG(MultiTexCoordP1ui)(GLenum target, GLenum type, GLuint coords)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glMultiTexCoordP1ui");
   ATTR_UI(ctx, 1, type, 0, attr, coords);
}

static void GLAPIENTRY
TAG(MultiTexCoordP1uiv)(GLenum target, GLenum type, const GLuint *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glMultiTexCoordP1uiv");
   ATTR_UI(ctx, 1, type, 0, attr, coords[0]);
}

static void GLAPIENTRY
TAG(MultiTexCoordP2ui)(GLenum target, GLenum type, GLuint coords)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glMultiTexCoordP2ui");
   ATTR_UI(ctx, 2, type, 0, attr, coords);
}

static void GLAPIENTRY
TAG(MultiTexCoordP2uiv)(GLenum target, GLenum type, const GLuint *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glMultiTexCoordP2uiv");
   ATTR_UI(ctx, 2, type, 0, attr, coords[0]);
}

static void GLAPIENTRY
TAG(MultiTexCoordP3ui)(GLenum target, GLenum type, GLuint coords)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glMultiTexCoordP3ui");
   ATTR_UI(ctx, 3, type, 0, attr, coords);
}

static void GLAPIENTRY
TAG(MultiTexCoordP3uiv)(GLenum target, GLenum type, const GLuint *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glMultiTexCoordP3uiv");
   ATTR_UI(ctx, 3, type, 0, attr, coords[0]);
}

static void GLAPIENTRY
TAG(MultiTexCoordP4ui)(GLenum target, GLenum type, GLuint coords)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glMultiTexCoordP4ui");
   ATTR_UI(ctx, 4, type, 0, attr, coords);
}

static void GLAPIENTRY
TAG(MultiTexCoordP4uiv)(GLenum target, GLenum type, const GLuint *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint attr = (target & 0x7) + VBO_ATTRIB_TEX0;
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glMultiTexCoordP4uiv");
   ATTR_UI(ctx, 4, type, 0, attr, coords[0]);
}

static void GLAPIENTRY
TAG(NormalP3ui)(GLenum type, GLuint coords)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glNormalP3ui");
   ATTR_UI(ctx, 3, type, 1, VBO_ATTRIB_NORMAL, coords);
}

static void GLAPIENTRY
TAG(NormalP3uiv)(GLenum type, const GLuint *coords)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glNormalP3uiv");
   ATTR_UI(ctx, 3, type, 1, VBO_ATTRIB_NORMAL, coords[0]);
}

static void GLAPIENTRY
TAG(ColorP3ui)(GLenum type, GLuint color)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glColorP3ui");
   ATTR_UI(ctx, 3, type, 1, VBO_ATTRIB_COLOR0, color);
}

static void GLAPIENTRY
TAG(ColorP3uiv)(GLenum type, const GLuint *color)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glColorP3uiv");
   ATTR_UI(ctx, 3, type, 1, VBO_ATTRIB_COLOR0, color[0]);
}

static void GLAPIENTRY
TAG(ColorP4ui)(GLenum type, GLuint color)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glColorP4ui");
   ATTR_UI(ctx, 4, type, 1, VBO_ATTRIB_COLOR0, color);
}

static void GLAPIENTRY
TAG(ColorP4uiv)(GLenum type, const GLuint *color)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glColorP4uiv");
   ATTR_UI(ctx, 4, type, 1, VBO_ATTRIB_COLOR0, color[0]);
}

static void GLAPIENTRY
TAG(SecondaryColorP3ui)(GLenum type, GLuint color)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glSecondaryColorP3ui");
   ATTR_UI(ctx, 3, type, 1, VBO_ATTRIB_COLOR1, color);
}

static void GLAPIENTRY
TAG(SecondaryColorP3uiv)(GLenum type, const GLuint *color)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glSecondaryColorP3uiv");
   ATTR_UI(ctx, 3, type, 1, VBO_ATTRIB_COLOR1, color[0]);
}

static void GLAPIENTRY
TAG(VertexAttribP1ui)(GLuint index, GLenum type, GLboolean normalized,
		      GLuint value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE_EXT(ctx, type, "glVertexAttribP1ui");
   ATTR_UI_INDEX(ctx, 1, type, normalized, index, value);
}

static void GLAPIENTRY
TAG(VertexAttribP2ui)(GLuint index, GLenum type, GLboolean normalized,
		      GLuint value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE_EXT(ctx, type, "glVertexAttribP2ui");
   ATTR_UI_INDEX(ctx, 2, type, normalized, index, value);
}

static void GLAPIENTRY
TAG(VertexAttribP3ui)(GLuint index, GLenum type, GLboolean normalized,
		      GLuint value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE_EXT(ctx, type, "glVertexAttribP3ui");
   ATTR_UI_INDEX(ctx, 3, type, normalized, index, value);
}

static void GLAPIENTRY
TAG(VertexAttribP4ui)(GLuint index, GLenum type, GLboolean normalized,
		      GLuint value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glVertexAttribP4ui");
   ATTR_UI_INDEX(ctx, 4, type, normalized, index, value);
}

static void GLAPIENTRY
TAG(VertexAttribP1uiv)(GLuint index, GLenum type, GLboolean normalized,
		       const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE_EXT(ctx, type, "glVertexAttribP1uiv");
   ATTR_UI_INDEX(ctx, 1, type, normalized, index, *value);
}

static void GLAPIENTRY
TAG(VertexAttribP2uiv)(GLuint index, GLenum type, GLboolean normalized,
		       const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE_EXT(ctx, type, "glVertexAttribP2uiv");
   ATTR_UI_INDEX(ctx, 2, type, normalized, index, *value);
}

static void GLAPIENTRY
TAG(VertexAttribP3uiv)(GLuint index, GLenum type, GLboolean normalized,
		       const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE_EXT(ctx, type, "glVertexAttribP3uiv");
   ATTR_UI_INDEX(ctx, 3, type, normalized, index, *value);
}

static void GLAPIENTRY
TAG(VertexAttribP4uiv)(GLuint index, GLenum type, GLboolean normalized,
		      const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ERROR_IF_NOT_PACKED_TYPE(ctx, type, "glVertexAttribP4uiv");
   ATTR_UI_INDEX(ctx, 4, type, normalized, index, *value);
}



static void GLAPIENTRY
TAG(VertexAttribL1d)(GLuint index, GLdouble x)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR1D(0, x);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR1D(VBO_ATTRIB_GENERIC0 + index, x);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribL1dv)(GLuint index, const GLdouble * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR1DV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR1DV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribL2d)(GLuint index, GLdouble x, GLdouble y)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR2D(0, x, y);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR2D(VBO_ATTRIB_GENERIC0 + index, x, y);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribL2dv)(GLuint index, const GLdouble * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR2DV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR2DV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribL3d)(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR3D(0, x, y, z);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR3D(VBO_ATTRIB_GENERIC0 + index, x, y, z);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribL3dv)(GLuint index, const GLdouble * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR3DV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR3DV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribL4d)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR4D(0, x, y, z, w);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR4D(VBO_ATTRIB_GENERIC0 + index, x, y, z, w);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribL4dv)(GLuint index, const GLdouble * v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR4DV(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR4DV(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribL1ui64ARB)(GLuint index, GLuint64EXT x)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR1UI64(0, x);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR1UI64(VBO_ATTRIB_GENERIC0 + index, x);
   else
      ERROR(GL_INVALID_VALUE);
}

static void GLAPIENTRY
TAG(VertexAttribL1ui64vARB)(GLuint index, const GLuint64EXT *v)
{
   GET_CURRENT_CONTEXT(ctx);
   if (is_vertex_position(ctx, index))
      ATTR1UIV64(0, v);
   else if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR1UIV64(VBO_ATTRIB_GENERIC0 + index, v);
   else
      ERROR(GL_INVALID_VALUE);
}

#undef ATTR1FV
#undef ATTR2FV
#undef ATTR3FV
#undef ATTR4FV

#undef ATTR1F
#undef ATTR2F
#undef ATTR3F
#undef ATTR4F

#undef ATTR_UI

#undef MAT
