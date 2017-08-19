float PI = 3.141592;

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

vec2 moda(vec2 uv, float n)
{
    float repeat = PI / n;
    float angle = atan(uv.y, uv.x);
    return rotate(uv, mod(angle + repeat, 2.0 * repeat) - repeat - angle);
}

float box(vec3 pos, vec3 size)
{
    vec3 diff = abs(pos) - size;
    return length(max(diff, 0.0)) + vmax(min(diff, 0.0));
}

float torus(vec3 pos, vec2 radiuses)
{
    return nnorm(vec2(length(pos.xy) - radiuses.x, pos.z), 8.0) - radiuses.y;
}

float cog(vec3 pos, float id)
{
    float d = torus(pos, vec2(2.0, 0.1));
    pos.xy = moda(pos.xy, 4.0 + floor(id * 6.0));
    pos.x -= 2.2;
    return min(d, box(pos, vec3(0.2, 0.3, 0.1)));
}

float cogs(vec3 pos)
{
    float instance;
    pos.z = repeat(pos.z, 4.0, instance);
    float id = rand(instance * 47.0);
    pos.xy = rotate(pos.xy,  (id - 0.5) * iGlobalTime * 3.0);
    return cog(pos, id);
}

float panels(vec3 pos)
{
    float instance;
    pos.z += iGlobalTime * 1.3;
    pos.z = repeat(pos.z, 2.0, instance);
    pos.xy = rotate(pos.xy,  (rand(instance) - 0.5) * iGlobalTime * 0.2);
    pos.xy = moda(pos.xy, 7.0);
    pos.x -= 3.0;
    return min(box(pos, vec3(0.02, 0.4, 0.4)), box(pos, vec3(0.04, 0.2, 0.7)));
}

float panels2(vec3 pos)
{
    float instance;
    pos.z += iGlobalTime * 2.7;
    pos.z = repeat(pos.z, 3.0, instance);
    pos.xy = rotate(pos.xy,  (rand(instance) - 0.5) * iGlobalTime * 0.7);
    pos.xy = moda(pos.xy, 5.0);
    pos.x -= 1.7;
    return box(pos, vec3(0.01, 0.5, 0.8));
}

float map(vec3 pos)
{
    //pos.xy = rotate(pos.xy, iGlobalTime * 0.7);
    //pos.yz = rotate(pos.yz, iGlobalTime);
    //return min(cogs(pos), -length(pos.xy) + 5.0);
    //return cogs(pos);
    return min(min(min(cogs(pos), panels(pos)), panels2(pos)), 8.0 - length(pos.xy));
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

vec3 tonemap(vec3 color)
{
	// rheinhard
	color = color / (1.0 + color);
	
	// gamma
	color = pow(color, vec3(1.0 / 2.2));
	
	return color;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = (fragCoord.xy - iResolution.xy * 0.5) / iResolution.y;
    
	vec3 dir = normalize(vec3(uv, 1.0));
	vec3 pos = vec3(0.0, 0.0, iGlobalTime);
	float d;
	int i;
	for (i = 0; i < 64; i++)
	{
		d = map(pos);
		if (d < 0.001) break;
		pos += dir * d;
	}
    
    //vec3 light = vec3(20.0, 0.0, 0.0) / (length(pos) * length(pos) * 100.0 + 1.0);
	fragColor = vec4(tonemap(normal(pos) * 0.5 + 0.5),1.0);
}