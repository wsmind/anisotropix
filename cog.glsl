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

vec2 moda(vec2 uv, float n)
{
    float repeat = PI / n;
    float angle = atan(uv.y, uv.x);
    return rotate(uv, mod(angle + repeat, 2.0 * repeat) - repeat - angle);
}

float cube(vec3 pos, vec3 size)
{
    vec3 diff = abs(pos) - size;
    return length(max(diff, 0.0)) + vmax(min(diff, 0.0));
}

float torus(vec3 pos, vec2 radiuses)
{
    return nnorm(vec2(length(pos.xy) - radiuses.x, pos.z), 8.0) - radiuses.y;
}

float cog(vec3 pos)
{
    float d = torus(pos, vec2(1.0, 0.2));
    pos.xy = moda(pos.xy, 6.0 /*+ sin(iGlobalTime) * 2.0*/);
    pos.x -= 1.3;
    return min(d, cube(pos, vec3(0.2, 0.3, 0.2)));
}

float cogs(vec3 pos)
{
    float instance = floor((pos.z + 2.0) / 4.0);
    pos.z = mod(pos.z + 2.0, 4.0) - 2.0;
    pos.xy = rotate(pos.xy,  (rand(instance * 47.0) - 0.5) * iGlobalTime * 3.0);
    return cog(pos);
}

float map(vec3 pos)
{
    //pos.xy = rotate(pos.xy, iGlobalTime * 0.7);
    //pos.yz = rotate(pos.yz, iGlobalTime);
    //return min(cogs(pos), -length(pos.xy) + 5.0);
    return min(5.0 - length(pos.xy), cogs(pos));
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