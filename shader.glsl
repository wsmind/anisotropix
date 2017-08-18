//! FRAGMENT

uniform float _u[UNIFORM_COUNT];
vec2 resolution = vec2(_u[4], _u[5]);

vec2 rotate(vec2 uv, float a)
{
	return mat2(cos(a), sin(a), -sin(a), cos(a)) * uv;
}

float sphere(vec3 pos, float radius)
{
	return length(pos)-radius;
}

float DE(vec3 pos, int iterations, float details, float power) 
{
	vec3 z = pos;
	float dr = 1.0;
	float r = 0.0;
	for (int i = 0; i < iterations ; i++) {
		r = length(z);
		if (r>details) break;
		
		// convert to polar coordinates
		float theta = acos(z.z/r);
		float phi = atan(z.y,z.x);
		dr =  pow(r, power -1.0)*power*dr + 1.0;
		
		// scale and rotate the point
		float zr = pow(r,power);
		theta = theta*power;
		phi = phi*power;
		
		// convert back to cartesian coordinates
		z = zr*vec3(sin(theta)*cos(phi), sin(phi)*sin(theta), cos(theta));
		z+=pos;
	}
	return 0.5*log(r)*r/dr;
}

float map(vec3 pos)
{
	int iterations = 8;
    float details = smoothstep(10.,20.,_u[0])*2.;
    float bounces = 0.4;
    float power = 3.;
	
    pos.xy = rotate(pos.xy, _u[0] * 0.7);
    pos.xz = rotate(pos.xz, _u[0]);
    return DE(pos*(abs(sin(_u[0]/bounces))*0.3+0.7),iterations,details,power);
}

vec3 normal(vec3 p)
{
	vec2 e = vec2(0.06, 0.0);
	return normalize(vec3(
		map(p + e.xyy) - map(p - e.xyy),
		map(p + e.yxy) - map(p - e.yxy),
		map(p + e.yyx) - map(p - e.yyx)
	));
}

vec3 tonemap(vec3 color)
{
	// rheinhard
	color = color / (1.0 + color);	
	// gamma
	color = pow(color, vec3(1.0 / 2.2));	
	return color;
}

float lighting (vec3 norm)
{
    vec3 lightdir = vec3(0.,1.,0.1);
    return dot(norm,lightdir)*0.5+0.5;
}

void main(void)
{	
	vec2 uv = vec2(gl_FragCoord.xy - resolution.xy * 0.5) / resolution.y;

	vec3 dir = normalize(vec3(uv, 0.5 - length(uv) * 0.4));
	vec3 pos = vec3(0., 0., -2.0);
	vec3 color = vec3(1.0);
	float d;
	int i;
	for (i = 0; i < 64; i++)
	{
		d = map(pos);
		if (d < 0.001) 
		{
			vec3 norm = normal(pos);
            color = vec3(lighting(norm));
            break;
		}
		pos += dir * d;
	}
	
	color = tonemap(color);
	
	// white flashes
	/*float flash = exp(-mod(_u[0] - 4.0, 32.0) * 0.5);
	color = mix(color, vec3(1.0), flash);*/
	
	gl_FragColor = vec4(color,1.0);
}
