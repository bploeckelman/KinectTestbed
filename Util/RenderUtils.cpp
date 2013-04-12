/************************************************************************/
/* Render 
/* ------
/* A static helper class for rendering various things 
/************************************************************************/
#include "RenderUtils.h"
#include "ImageManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/Texture.hpp>

using glm::vec3;
using glm::vec4;
using glm::value_ptr;


void Render::cube( const vec3& position, const float scale )
{
	glPushMatrix();
	glTranslated(position.x, position.y, position.z);

	const float nl = -0.5f * scale;
	const float pl =  0.5f * scale;

	glBegin(GL_QUADS);
		glNormal3d( 0,0,1);
			glVertex3d(pl,pl,pl);
			glVertex3d(nl,pl,pl);
			glVertex3d(nl,nl,pl);
			glVertex3d(pl,nl,pl);
		glNormal3d( 0, 0, -1);
			glVertex3d(pl,pl,nl);
			glVertex3d(pl,nl,nl);
			glVertex3d(nl,nl,nl);
			glVertex3d(nl,pl,nl);
		glNormal3d( 0, 1, 0);
			glVertex3d(pl,pl,pl);
			glVertex3d(pl,pl,nl);
			glVertex3d(nl,pl,nl);
			glVertex3d(nl,pl,pl);
		glNormal3d( 0,-1,0);
			glVertex3d(pl,nl,pl);
			glVertex3d(nl,nl,pl);
			glVertex3d(nl,nl,nl);
			glVertex3d(pl,nl,nl);
		glNormal3d( 1,0,0);
			glVertex3d(pl,pl,pl);
			glVertex3d(pl,nl,pl);
			glVertex3d(pl,nl,nl);
			glVertex3d(pl,pl,nl);
		glNormal3d(-1,0,0);
			glVertex3d(nl,pl,pl);
			glVertex3d(nl,pl,nl);
			glVertex3d(nl,nl,nl);
			glVertex3d(nl,nl,pl);
	glEnd();
	
	glPopMatrix(); 

	glColor3f(1,1,1);
}

void Render::pyramid( const vec3& pos, const float radius, const float height )
{
	glPushMatrix();
	glTranslated(pos.x, pos.y, pos.z);

	glPushMatrix();
		glRotatef(180, 1, 0, 0);
		glBegin(GL_QUADS);
			glVertex3d(-radius, 0, -radius);
			glVertex3d(-radius, 0,  radius);
			glVertex3d( radius, 0,  radius);
			glVertex3d( radius, 0, -radius);
		glEnd();
	glPopMatrix();

	glBegin(GL_TRIANGLE_FAN);
		glVertex3d(0, height, 0);
		glVertex3d(-radius, 0, -radius);
		glVertex3d(-radius, 0,  radius);
		glVertex3d( radius, 0,  radius);
		glVertex3d( radius, 0, -radius);
		glVertex3d(-radius, 0, -radius);
	glEnd();

	glPopMatrix();
}

void Render::vector( const glm::vec3& vec, const glm::vec3& point , const glm::vec4& color/*=glm::vec4(1,1,1,1)*/ )
{
	glColor4fv(value_ptr(color));

	glBegin(GL_LINES);
		glVertex3fv(value_ptr(point));
		glVertex3fv(value_ptr(point + vec));
	glEnd();

	glBegin(GL_POINTS);
		glVertex3fv(value_ptr(point + vec));
	glEnd();

	glColor4f(1,1,1,1);
}

void Render::basis( const float scale/*=1.f */
				  , const vec3& pos/*=vec3(0,0,0) */
				  , const vec3& x/*=vec3(1,0,0) */
				  , const vec3& y/*=vec3(0,1,0) */
				  , const vec3& z/*=vec3(0,0,1)*/
				  , const float alpha/*=1.f*/ )
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	//static const vec3 origin(0,0,0);
	const vec4 red   (1,0,0,alpha);
	const vec4 green (0,1,0,alpha);
	const vec4 blue  (0,0,1,alpha);

	vector(x * scale, pos, red);
	vector(y * scale, pos, green);
	vector(z * scale, pos, blue);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
}

void Render::ground( const float alpha )
{
	static const float Y = 0.f;
	static const float R = 3.f;

	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	sf::Texture texture;
	texture.loadFromImage(GetImage("grid.png"));
	texture.bind(&texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	const float radius = 10.f;
	glColor4f(1,1,1,alpha);
	glBegin(GL_TRIANGLE_STRIP);
		glNormal3f(0, 1, 0); glTexCoord2f(   0.f,    0.f); glVertex3f( R, Y,  R);
		glNormal3f(0, 1, 0); glTexCoord2f(radius,    0.f); glVertex3f( R, Y, -R);
		glNormal3f(0, 1, 0); glTexCoord2f(   0.f, radius); glVertex3f(-R, Y,  R);
		glNormal3f(0, 1, 0); glTexCoord2f(radius, radius); glVertex3f(-R, Y, -R);
	glEnd();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
}
