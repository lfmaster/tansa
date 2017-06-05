/* My implementation of the OpenGL Mathematics Library */

#ifndef TANSA_GRAPHICS_GLM_H_
#define TANSA_GRAPHICS_GLM_H_

#include <GL/glew.h>

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <cmath>


namespace tansa {
namespace graphics {
namespace glm {

struct vec2 {
	vec2(GLfloat x = 0, GLfloat y = 0){ this->x = x; this->y = y; };

	union {
		struct {
			GLfloat x;
			GLfloat y;
		};
		GLfloat value[2];
	};

};


struct vec3 {
	vec3(GLfloat x = 0, GLfloat y = 0, GLfloat z = 0){ this->x = x; this->y = y; this->z = z; }

	vec3(const GLfloat *v) { this->x = v[0]; this->y = v[1]; this->z = v[2]; }

	union {
		struct {
			GLfloat x;
			GLfloat y;
			GLfloat z;
		};
		GLfloat value[3];
	};

	void operator+=(const vec3 &other) {
		this->x += other.x;
		this->y += other.y;
		this->z += other.z;
	}

};

inline vec3 operator-(const vec3 &lhs, const vec3 &rhs){
	return vec3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}


inline vec3 operator*(GLfloat s, const vec3 &rhs){
	return vec3(rhs.x*s, rhs.y*s, rhs.z*s);
}


struct vec4 {
	vec4(GLfloat x = 0, GLfloat y = 0, GLfloat z = 0, GLfloat w = 0){ this->x = x; this->y = y; this->z = z; this->w = w; };
	vec4(const vec3 &v, GLfloat w) : vec4(v.x, v.y, v.z, w){};

	GLfloat operator[](int i){
		return value[i];
	};

	union {
		struct {
			GLfloat x;
			GLfloat y;
			GLfloat z;
			GLfloat w;
		};
		GLfloat value[3];
	};


};

struct mat4 {

	mat4(vec4 a, vec4 b, vec4 c, vec4 d){
		value[0] = a;
		value[1] = b;
		value[2] = c;
		value[3] = d;
	}

	mat4(GLfloat v = 1.0f){
		value[0] = vec4(v, 0, 0, 0);
		value[1] = vec4(0, v, 0, 0);
		value[2] = vec4(0, 0, v, 0);
		value[3] = vec4(0, 0, 0, v);
	};

	mat4(GLfloat m00, GLfloat m01, GLfloat m02, GLfloat m03,
		GLfloat m10, GLfloat m11, GLfloat m12, GLfloat m13,
		GLfloat m20, GLfloat m21, GLfloat m22, GLfloat m23,
		GLfloat m30, GLfloat m31, GLfloat m32, GLfloat m33
	){

		value[0] = vec4(m00, m10, m20, m30);
		value[1] = vec4(m01, m11, m21, m31);
		value[2] = vec4(m02, m12, m22, m32);
		value[3] = vec4(m03, m13, m23, m33);
	};


	vec4 value[4];
};


inline vec4 operator*(const mat4 &lhs, const vec4 &rhs){
	return vec4(
		(lhs.value[0].x * rhs.x) + (lhs.value[1].x * rhs.y) + (lhs.value[2].x * rhs.z) + (lhs.value[3].x * rhs.w),
		(lhs.value[0].y * rhs.x) + (lhs.value[1].y * rhs.y) + (lhs.value[2].y * rhs.z) + (lhs.value[3].y * rhs.w),
		(lhs.value[0].z * rhs.x) + (lhs.value[1].z * rhs.y) + (lhs.value[2].z * rhs.z) + (lhs.value[3].z * rhs.w),
		(lhs.value[0].w * rhs.x) + (lhs.value[1].w * rhs.y) + (lhs.value[2].w * rhs.z) + (lhs.value[3].w * rhs.w)
	);
}

inline mat4 operator*(const mat4 &lhs, const mat4 &rhs){
	mat4 m;
	m.value[0] = lhs * rhs.value[0];
	m.value[1] = lhs * rhs.value[1];
	m.value[2] = lhs * rhs.value[2];
	m.value[3] = lhs * rhs.value[3];
	return m;
}



inline const GLfloat *value_ptr(const vec3 &m){ return m.value; };
inline const GLfloat *value_ptr(const vec4 &m){ return m.value; };
inline GLfloat *value_ptr(const mat4 &m){ return (GLfloat *)m.value; };









// Transforms

inline mat4 translate(vec3 v){
	return mat4(
		1, 0, 0, v.x,
		0, 1, 0, v.y,
		0, 0, 1, v.z,
		0, 0, 0, 1
	);
}

inline mat4 scale(vec3 v){
	return mat4(
		v.x, 0, 0, 0,
		0, v.y, 0, 0,
		0, 0, v.z, 0,
		0, 0, 0, 1
	);
}


inline mat4 rotate(GLfloat t, vec3 u){
	return mat4(
		cos(t) + u.x*u.x*(1.0 - cos(t)), u.x*u.y*(1.0 - cos(t)) - u.z*sin(t), u.x*u.z*(1.0 - cos(t)) + u.y*sin(t), 0,
		u.y*u.x*(1.0-cos(t)) + u.z*sin(t), cos(t) + u.y*u.y*(1.0 - cos(t)), u.y*u.z*(1.0 - cos(t)) - u.x*sin(t), 0,
		u.z*u.x*(1.0-cos(t)) - u.y*sin(t), u.z*u.y*(1.0 - cos(t)) + u.x*sin(t), cos(t) + u.z*u.z*(1.0 - cos(t)), 0,
		0, 0, 0, 1
	);
}


inline mat4 perspective(GLfloat fovy, GLfloat aspect, GLfloat near, GLfloat far){

	GLfloat top = tan(fovy/2) * near;
	GLfloat right = top * aspect;

	GLfloat a = -(far + near) / (far - near);
	GLfloat b = (-2.0*near*far) / (far - near);

	return mat4(
		near/right, 0, 0, 0,
		0, near/top, 0, 0,
		0, 0, a, b,
		0, 0, -1, 0
	);
}



inline mat4 ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar){

	return mat4(
		2.0/(right-left), 0, 0, -(right+left)/(right-left),
		0, 2.0/(top-bottom), 0, -(top+bottom)/(top-bottom),
		0, 0, -2.0/(zFar-zNear), -(zFar+zNear)/(zFar-zNear),
		0, 0, 0, 1
	);

}


inline vec3 cross(vec3 u, vec3 v){
	return vec3(
		u.y*v.z - u.z*v.y,
		u.z*v.x - u.x*v.z,
		u.x*v.y - u.y*v.x
	);
}

inline vec3 normalize(vec3 x){
	GLfloat n = sqrt(x.x*x.x + x.y*x.y + x.z*x.z);
	return vec3(x.x / n, x.y / n, x.z / n);
}


inline mat4 lookAt(vec3 eye, vec3 center, vec3 up){

	vec3 n = normalize(eye - center);
	vec3 uu = normalize(cross(up, n));
	vec4 u = vec4(uu.x, uu.y, uu.z, 0.0);
	vec3 vv = normalize(cross(n,uu));
	vec4 v = vec4(vv.x, vv.y, vv.z, 0.0);
	vec4 t = vec4(0.0, 0.0, 0.0, 1.0);

	mat4 c(
		u.x, u.y, u.z, 0,
		v.x, v.y, v.z, 0,
		n.x, n.y, n.z, 0,
		0, 0, 0, 1
	);

	return c * translate(vec3(-eye.x, -eye.y, -eye.z));
}


}
}
}




#endif