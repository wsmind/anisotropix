//! FRAGMENT

uniform float _u[UNIFORM_COUNT];
vec2 resolution = vec2(_u[4], _u[5]);

float saturation;
float holeAmount;
float crazy;
float laser;

float Scale = 2.0;
int Iterations = 6;

vec2 rotate(vec2 uv, float a)
{
	return mat2(cos(a), sin(a), -sin(a), cos(a)) * uv;
}

float fractal(vec3 z)
{
    vec3 pos = z;
    
	vec3 a1 = vec3(1,1,1);
	vec3 a2 = vec3(-1,-1,1);
	vec3 a3 = vec3(1,-1,-1);
	vec3 a4 = vec3(-1,1,-1);
	vec3 c;
	int n = 0;
	float dist, d;
	while (n < Iterations) {
		 c = a1; dist = length(z-a1);
	        d = length(z-a2); if (d < dist) { c = a2; dist=d; }
		 d = length(z-a3); if (d < dist) { c = a3; dist=d; }
		 d = length(z-a4); if (d < dist) { c = a4; dist=d; }
		z = Scale*z-c*(Scale-1.0);
		n++;
	}
 
	return length(z) * pow(Scale, float(-n)) - sin(pos.y * 3.0 + _u[0] * 10.0) * 0.01 - 0.01;
}

float map(vec3 pos)
{
    pos.xy = rotate(pos.xy, _u[0] * 0.7);
    pos.xz = rotate(pos.xz, _u[0]);
    return fractal(pos);
}

vec3 normal(vec3 p)
{
	vec2 e = vec2(0.01, 0.0);
	return normalize(vec3(
		map(p + e.xyy) - map(p - e.xyy),
		map(p + e.yxy) - map(p - e.yxy),
		map(p + e.yyx) - map(p - e.yyx)
	));
}

/*float ao(vec3 p, float md)
{
	vec3 n = normal(p);
	float d = map(p + n * md);
	return smoothstep(-md, md, d);
	//return clamp(d, 0.0, md) / md;
}*/

float ao(vec3 p, vec3 n, float step)
{
	float md = 0.0;
	float ao = 1.0;
	for (int i = 0; i < 5; i++)
	{
		p += n * step;
		md += step;
		ao = min(ao, map(p) / md);
		//ao = min(ao, smoothstep(0.0, md * 2.0, map(p)));
	}
	
	return max(ao, 0.0);
}

vec3 tonemap(vec3 color)
{
	// rheinhard
	color = color / (1.0 + color);
	
	// gamma
	color = pow(color, vec3(1.0 / 2.2));
	
	return color;
}

float light(vec3 p, vec3 n, float d, float range, float energy)
{
	float irradiance = dot(n.xy, -normalize(p.xy)) * 0.4 + 0.6;
	
	float ld = d / range;
	return irradiance * energy / (ld * ld + 1.0);
}

void main(void)
{
	saturation = step(36.0, _u[0]) * step(0.0, 260.0 - _u[0]) + step(292.0, _u[0]); //sin(_u[0] * 0.5) * 0.5 + 0.5;
	holeAmount = smoothstep(132.0, 164.0, _u[0]) * step(0.0, 260.0 - _u[0]); //sin(_u[0] * 0.1) * 0.5 + 0.5;
	crazy = smoothstep(194.0, 196.0, _u[0]); //sin(_u[0] * 0.2) * 0.5 + 0.5;
	laser = fract(_u[3] * 0.5); /*sin(_u[0] * 0.8) * 0.5 + 0.5;*/
	
	float shake = 0.0; //(-exp(-mod(_u[0] - 4.0, 32.0) * 4.0) + laser * 0.02) * rand(_u[0]);
	
	vec2 uv = vec2(gl_FragCoord.xy - resolution.xy * 0.5) / resolution.y;
	uv = rotate(uv, sin(_u[0] * 0.2) * 0.1 + (crazy * fract(_u[0] * 0.01) + shake) * 6.28);
	
	vec3 dir = normalize(vec3(uv, 0.5 - length(uv) * 0.4));
	vec3 pos = vec3(shake, -shake, -2.0);
	float d;
	int i;
	for (i = 0; i < 64; i++)
	{
		d = map(pos);
		if (d < 0.001) break;
		pos += dir * d;
	}

	//vec3 color = vec3(float(i) / 64.0);
	//vec3 color = normal(pos);
	vec3 n = normal(pos);
	float occ = ao(pos, n, 0.04);
	//vec3 color = (ao(pos, n, 0.04) * 0.5 + 0.5);// * (normal(pos) * 0.5 + 0.5);
	/*vec3 sphereLight = mix(vec3(1.0), vec3(1.0, 0.1, 0.0), saturation) * light(pos, n, spheres(pos), 0.2, 2.0);
	vec3 cubeLight = mix(vec3(1.0), vec3(0.0, 0.7, 1.0), saturation) * light(pos, n, cubes2(pos), 0.1, 4.0) * exp(-fract(_u[0] * 0.5) * 4.0);
	vec3 holeLight = vec3(1.0, 0.02, 0.0) * light(pos, n, pos.y + mix(2.0, sin(_u[0] * 0.2) * 20.0, crazy) + holeAmount * 2.0, 0.3, 10.0) * holeAmount;
	vec3 tubeLight = vec3(10.0, 0.0, 0.02) * light(pos, n, tubes(pos), 0.2 + laser * 0.2, laser);
	vec3 radiance = sphereLight + cubeLight + holeLight + tubeLight + occ * 0.01;*/
	vec3 radiance = vec3(occ);
	vec3 color = tonemap(radiance);
	
	// white flashes
	float flash = exp(-mod(_u[0] - 4.0, 32.0) * 0.5);
	color = mix(color, vec3(1.0), flash);
	
	// fade to black
	color = mix(color, vec3(0.0), _u[1]);
	
	gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);
}
