//! FRAGMENT

uniform float _u[UNIFORM_COUNT];
vec2 resolution = vec2(_u[4], _u[5]);

float PI = 3.141592;

float crazy_corner;
float crazy_radius;
float arm_rotation;
int arm_iteration;

float pulse;
float kick_pulse;
float lead_pulse;

float radius_animation;
float starlights_visible;
float cogs_visible;

vec2 rotate(vec2 uv, float a)
{
	return mat2(cos(a), sin(a), -sin(a), cos(a)) * uv;
}

float vmax(vec3 pos)
{
    return max(max(pos.x, pos.y), pos.z);
}

float nnorm(vec2 uv, float n)
{
    return pow(pow(uv.x, n) + pow(uv.y, n), 1.0 / n);
}

float rand(float f)
{
	return fract(sin(f * 12.9898) * 43758.5453);
}

float repeat(float f, float period, out float instance)
{
    instance = floor((f + period * 0.5) / period);
    return mod(f + period * 0.5, period) - period * 0.5;
}

vec2 moda(vec2 uv, float n, out float index)
{
    float period = PI / n;
    float angle = atan(uv.y, uv.x);
	index = floor((angle + period) / 2.0 * period);
    return rotate(uv, mod(angle + period, 2.0 * period) - period - angle);
}

float box(vec3 pos, vec3 size)
{
    vec3 diff = abs(pos) - size;
    return length(max(diff, 0.0)) + vmax(min(diff, 0.0));
}

float torus(vec3 pos, vec2 radiuses)
{
    return nnorm(vec2(length(pos.xy) - radiuses.x, pos.z), 2.0) - radiuses.y;
}

float cog(vec3 pos, float id)
{
    float d = torus(pos, vec2(2.0, 0.3));
	float index;
    pos.xy = moda(pos.xy, 4.0 + floor(id * 6.0), index);
    pos.x -= 1.7;
    return min(d, box(pos, vec3(0.45, 0.3, 0.04)));
}

float cogs(vec3 pos)
{
	float cogPeriod = 1.0;
	
	pos.x += 1000.0 * (1.0 - cogs_visible);
	
    float instance;
    pos.z += _u[0] * 1.7;
    pos.z = repeat(pos.z, 4.0, instance);
    float id = rand(instance * 47.0);
	float cogTime = _u[0] / cogPeriod;
	cogTime = floor(cogTime) + pow(fract(cogTime), 4.0);
	cogTime *= 3.0 * cogPeriod;
    pos.xy = rotate(pos.xy, (id - 0.5) * cogTime);
    return cog(pos, id);
}

float panels(vec3 pos)
{
    float instance, index;
    pos.z += _u[0] * 2.7;
    pos.z = repeat(pos.z, 7.0, instance);
    pos.xy = rotate(pos.xy,  (rand(instance) - 0.5) * _u[0] * 0.2);
    pos.xy = moda(pos.xy, 7.0, index);
    pos.x -= 3.0;
    return box(pos, vec3(0.04, 0.02, 0.7) * starlights_visible);
}

/*float panels2(vec3 pos)
{
    float instance, index;
    //pos.xy = rotate(pos.xy,  (rand(instance) - 0.5) * _u[0] * 0.9);
    pos.xy = moda(pos.xy, 5.0, index);
    pos.z += _u[0] * 3.7 + rand(index) * 2.0;
    pos.z = repeat(pos.z, 9.0, instance);
    pos.x -= 1.15;
    return box(pos, vec3(0.01, 0.01, 0.8));
}*/

float panels2(vec3 pos)
{
    float instance;
    pos.z += _u[0] * 3.7;
    pos.z = repeat(pos.z, 17.0, instance);
	
	float d = 1000000.0;
	
	for (int i = 0; i < 5; i++)
	{
		vec3 p = pos;
		p.xy = rotate(p.xy, float(i) * PI * 2.0 / 5.0);
		p.x -= 1.15;
		p.z += rand(float(i)) * 6.0;
		d = min(d, box(p, vec3(0.01, 0.01, 0.8) * starlights_visible));
	}
	
	return d;
}

float sphere(vec3 pos, float radius)
{
	return length(pos) - radius;
}


float prim(vec3 pos, vec3 corner, float radius)
{
    return max(-sphere(pos,radius),box(pos, corner));
}

float frac_octopus(vec3 pos)
{
	pos.xy = rotate(pos.xy, _u[0]*0.7+tan((_u[0]-16.)*PI/32.)*0.05);
	pos.xz = rotate(pos.xz,sin(_u[0]));
    pos = abs(pos);
    float corner = 0.5;
	float radius = crazy_radius * corner;
    float c = prim(pos,vec3(corner),radius);
    for (int i=0; i < arm_iteration; i++)
    {
        pos = pos-vec3(corner);
        corner *= crazy_corner;
		radius *= crazy_corner;
        pos.xy = rotate(pos.xy, arm_rotation);
        pos.yz = rotate(pos.yz,	arm_rotation);
        float b = prim(pos,vec3(corner),radius);
        c = min(b,c);
    }
    return c;
}

float map(vec3 pos)
{
    //pos.xy = rotate(pos.xy, _u[0] * 0.7);
    //pos.yz = rotate(pos.yz, _u[0]);
    //return min(cogs(pos), -length(pos.xy) + 5.0);
    //return cogs(pos);
	//return frac_octopus(pos);
    return min(min(min(min(cogs(pos), panels(pos)), panels2(pos)), 8.0 - length(pos.xy)),frac_octopus(pos));
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

float light(vec3 p, vec3 n, float d, float range)
{
	vec3 lightDirection = normalize(vec3(1.0, 1.0, -1.0));
	//float irradiance = dot(n.xy, -normalize(p.xy)) * 0.4 + 0.6;
	float irradiance = dot(n, lightDirection) * 0.5 + 0.5;
	
	float ld = d / range;
	return irradiance / (ld * ld + 1.0);
}

vec3 tonemap(vec3 color)
{
	// rheinhard
	color = color / (1.0 + color);
	
	// gamma
	color = pow(color, vec3(1.0 / 2.2));
	
	return color;
}

/*float lighting (vec3 norm)
{
    vec3 lightdir = vec3(0.,1.,0.1);
    return dot(norm,lightdir)*0.5+0.5;
}*/

void main(void)
{
	pulse = exp(-mod(_u[0], 4.0) * 2.0);
	
	kick_pulse = mod(_u[0], 8.0);
	kick_pulse = exp(-fract(kick_pulse) * 8.0) * step(0.0, 2.0 - kick_pulse);

	lead_pulse = exp(-mod(_u[0] - 2.0, 8.0 + step(66.0, _u[0]) * 8.0) * 2.0) * step(0.0, 320.0 - _u[0]);
	
	radius_animation = smoothstep(60.0,64.0,_u[0]);
	starlights_visible = smoothstep(56.0, 64.0, _u[0]);
	cogs_visible = step(128.0, _u[0]);
	
	//crazy_radius = sin(_u[0]);
	//crazy_corner = clamp(sin(_u[0]) * 0.7, 0.5, 0.95);
	//arm_iteration = int(clamp(floor(_u[0] * 10.0 / 64.0), 1.0, 6.0));
	crazy_radius = mix(1.9, 1.4, pow(lead_pulse,1./4.))*(1.-radius_animation)+smoothstep(188.0,192.0,_u[0]);
	crazy_corner = 0.7;
	arm_iteration = int(clamp(floor(_u[0] * 10.0 / 64.0), 1.0, 8.0));
	arm_rotation = sin(_u[0]/4.);
	
	vec2 uv = vec2(gl_FragCoord.xy - resolution.xy * 0.5) / resolution.y;

	vec3 dir = normalize(vec3(uv, 0.5 - length(uv) * 0.4));
	vec3 pos = vec3(0.0, 0.0, -3.0);
	
	for (int i = 0; i < 128; i++)
	{
		float d = map(pos);
		if (d < 0.001) break;
		pos += dir * d;
	}
	
    vec3 n = normal(pos);
    float occlusion = ao(pos, normal(pos), 0.04);
	vec3 lightOctopus = mix(lead_pulse, 1.0, smoothstep(120.0, 128.0, _u[0])) * 0.6 * vec3(0.4, 7.0, 1.2) * light(pos, n, frac_octopus(pos), 0.1);
	vec3 lightPanels = starlights_visible * kick_pulse * vec3(7.0) * light(pos, n, panels(pos), 0.3);
	vec3 lightPanels2 = starlights_visible * vec3(3.0, 0.4, 3.0) * light(pos, n, panels2(pos), 0.1);
    
    vec3 radiance = lightOctopus + lightPanels + lightPanels2 + occlusion * vec3(0.001, 0.002, 0.002);
    
    float fog = exp(min((-pos.z + 2.0), 0.0) * 0.1);
    radiance = mix(vec3(0.0), radiance, fog);
	
	gl_FragColor = vec4(tonemap(radiance), 1.0);
}
