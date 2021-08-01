#version 330 compatibility

uniform float	uHeightAdjust;
uniform bool	uDisplacementMapping;
uniform bool	uBumpMapping;
uniform bool	uLighting;

in vec3 vMCposition;
in vec3 vECposition;
in vec3 vNormal;
in vec2 vST;

uniform float uShipPositionX, uShipPositionY, uShipPositionZ;
uniform float uHeightDelta;
uniform float uWidthDelta;
uniform sampler2D uTexUnit;
uniform sampler2D uTexDispUnit;

//const float LNGMIN = -1737.4/2.; // in meters, same as heights
//const float LNGMAX = 1737.4/2.;
//const float LATMIN = -1737.4/2.;
//const float LATMAX = 1737.4/2.;

float LNGMIN = -1.f / uWidthDelta;
float LNGMAX = 1.f / uWidthDelta;
float LATMIN = -1.f / uHeightDelta;
float LATMAX = 1.f / uHeightDelta;

void
main( )
{
	vec3 fragmentColor = texture( uTexUnit, vST ).rgb;
	if(uLighting)
	{
		vec3 shipPosition = vec3(uShipPositionX, uShipPositionY, uShipPositionZ);
		vec3 normal = vNormal;
		if(uDisplacementMapping || uBumpMapping)
		{
			vec2 stp0 = vec2( uWidthDelta, 0. );
			vec2 st0p = vec2( 0. , uHeightDelta );
	
			float west = texture2D( uTexDispUnit, vST-stp0 ).r;
			float east = texture2D( uTexDispUnit, vST+stp0 ).r;
			float south = texture2D( uTexDispUnit, vST-st0p ).r;
			float north = texture2D( uTexDispUnit, vST+st0p ).r;

			vec3 stangent = vec3( 2.*uWidthDelta*(LNGMAX-LNGMIN), 0., uHeightAdjust * ( east - west ) );
			vec3 ttangent = vec3( 0., 2.*uHeightDelta*(LATMAX-LATMIN), uHeightAdjust * ( north - south ) );
			normal = normalize( cross( stangent, ttangent ) );
		}
		
		float LightIntensity = dot( normalize( vec3(0.f, 0.f, 2000.f)-vMCposition), normal);
		if( LightIntensity < 0.1 )
			LightIntensity = 0.1;
		vec3 sceneLight = LightIntensity * fragmentColor;

		//Ship Light
		vec3 lightColor = vec3(1.f, 0.f, 0.f);
		vec3 ambiant = vec3(0.f, 0.f, 0.f);
		vec3 diffuse = vec3(0.f, 0.f, 0.f);
		float distance = distance(shipPosition, vMCposition);
		float maxDistance = 250.f;
		if(distance < maxDistance)
		{
			ambiant = fragmentColor * lightColor * pow(((maxDistance - distance)/maxDistance), 2.f);
			//float s = 0.;
			//vec3 light = shipPosition-vMCposition;
			//float d = max( dot(normal,light), 0. ); // only do diffuse if the light can see the point
			//diffuse = .01 * d * fragmentColor;
		}
		
		gl_FragColor = vec4( (ambiant + diffuse) + sceneLight, 1. );
	}
	else
	{
		gl_FragColor = vec4(fragmentColor, 1.f);
	}
}