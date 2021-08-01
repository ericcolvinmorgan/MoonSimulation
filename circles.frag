#version 330 compatibility

uniform float   uKa, uKd, uKs;		// coefficients of each type of lighting
uniform vec3  uColor;			// object color
uniform vec3  uSpecularColor;		// light color
uniform float   uShininess;		// specular exponent
uniform float	uf_time_radian;

in  vec2  vST;			// texture coords
in  vec3  vN;			// normal vector
in  vec3  vL;			// vector from point to light
in  vec3  vE;			// vector from point to eye

const int numberCircles = 32;
float radius = (1.f / float(numberCircles)) / 2.;

void
main( )
{
vec3 Normal = normalize(vN);
vec3 Light     = normalize(vL);
vec3 Eye        = normalize(vE);

vec3 myColor = uColor;
float currentCenter[] = {0.f, 0.f};

float currentS = vST.s;
float currentT = vST.t;
float compRadius = pow(radius * cos(uf_time_radian), 2);

for(int s = 0; s < numberCircles; s++)
	{	
		currentCenter[0] += radius;
		currentCenter[1] = 0.;
		for(int t = 0; t < numberCircles; t++)
		{
			currentCenter[1] += radius;
			if( (pow((currentS - currentCenter[0]), 2) + pow((currentT - currentCenter[1]), 2)) <= (compRadius) )
{
myColor = vec3( 1., 0., 0. );
}
currentCenter[1] += radius;
}
currentCenter[0] += radius;
}

vec3 ambient = uKa * myColor;

float d = max( dot(Normal,Light), 0. );       // only do diffuse if the light can see the point
vec3 diffuse = uKd * d * myColor;

float s = 0.;
if( dot(Normal,Light) > 0. )	          // only do specular if the light can see the point
{
vec3 ref = normalize(  reflect( -Light, Normal )  );
s = pow( max( dot(Eye,ref),0. ), uShininess );
}
vec3 specular = uKs * s * uSpecularColor;
gl_FragColor = vec4( ambient + diffuse + specular,  1. );
}