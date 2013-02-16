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

void Render::vector( const vec3& vec, const vec3& point, const vec3& color )
{
    glColor4fv(value_ptr(color));

    glBegin(GL_LINES);
        glVertex3fv(value_ptr(point));
        glVertex3fv(value_ptr(point + vec));
    glEnd();

    glBegin(GL_POINTS);
        glVertex3fv(value_ptr(point + vec));
    glEnd();

    glColor3f(1,1,1);
}

void Render::basis(const float scale
                 , const vec3& pos
                 , const vec3& x
                 , const vec3& y
                 , const vec3& z)
{
    glDisable(GL_TEXTURE_2D);

    //static const vec3 origin(0,0,0);
    static const vec3 red   (1,0,0);
    static const vec3 green (0,1,0);
    static const vec3 blue  (0,0,1);

    vector(x * scale, pos, red);
    vector(y * scale, pos, green);
    vector(z * scale, pos, blue);

    glEnable(GL_TEXTURE_2D);
}

void Render::ground()
{
    static const float Y = -1.f;
    static const float R = 10.f;

    glDisable(GL_CULL_FACE);

    sf::Texture texture;
    texture.loadFromImage(GetImage("grid.png"));
    texture.bind(&texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    const float radius = 50.f;
    glColor3f(1,1,1);
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0.f, 1.f, 0.f);
        glTexCoord2f(   0.f,    0.f); glVertex3f( R, Y,  R);
        glTexCoord2f(radius,    0.f); glVertex3f( R, Y, -R);
        glTexCoord2f(   0.f, radius); glVertex3f(-R, Y,  R);
        glTexCoord2f(radius, radius); glVertex3f(-R, Y, -R);
    glEnd();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_CULL_FACE);
}
