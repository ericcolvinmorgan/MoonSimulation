#version 330 compatibility

uniform float	uHeightAdjust;
uniform bool	uDisplacementMapping;
uniform sampler2D uTexDispUnit;

out vec3 vMCposition;
out vec3 vECposition;
out vec3 vNormal;
out vec2 vST;

void
main( )
{
	vST = gl_MultiTexCoord0.st;
	vNormal = normalize( gl_NormalMatrix * gl_Normal );
	if(!uDisplacementMapping)
	{
		vMCposition = gl_Vertex.xyz;
		vECposition = ( gl_ModelViewMatrix * gl_Vertex ).xyz;
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	}
	else
	{
		vec3 vert = gl_Vertex.xyz;
		vec4 disp = texture( uTexDispUnit, vST );
		float adjustment = disp.r * uHeightAdjust;

		vert.x = (vert.x * 1737.4f) + (vert.x * adjustment);
		vert.y = (vert.y * 1737.4f) + (vert.y * adjustment);
		vert.z = (vert.z * 1737.4f) + (vert.z * adjustment);

		vMCposition = vert;
		vECposition = (gl_ModelViewMatrix * vec4(vert, 1.)).xyz;
		gl_Position = gl_ModelViewProjectionMatrix * vec4( vert, 1. );
	}
}
