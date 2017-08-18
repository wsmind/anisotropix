vec2 rotate (float angle, vec2 pos)
{
    float c = cos(angle);
    float s = sin(angle);
    return (c,-s,s,c)*pos;
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

float sphere (vec3 pos, float radius)
{
    return length(pos)-radius;
}

float box(vec3 pos, vec3 corn)
{
    return length(max(abs(pos)-corn,0.));
}


float prim (vec3 pos)
{
    vec3 corn = vec3(0.7);
    float s = sphere(pos, 0.95);
    float b = box(pos,corn);
    return max(-s,b);
}

float back(vec3 pos)
{
    float angle = atan(pos.y,pos.x);
    float dist = length(pos.xy);
    float period = 2.*3.14/10.;
    
    angle = mod(angle+period/2.,period)-period/2.;
    pos = vec3(sin(angle)*dist,cos(angle)*dist,pos.z);
    pos.xy = mod(pos.xy+20.,40.)-20.;
    return prim(pos-sin(iGlobalTime));
}

float SDF(vec3 pos)
{
    int iterations = 10;
    float details = smoothstep(5.,10.,iGlobalTime)*2.;
    float bounces = 0.4;
    //float power = 50.;
    float power = (sin(iGlobalTime)+1.*0.5)*40.;
   
    float period = 2.;
    //pos.yz += rotate(abs(sin(iGlobalTime*0.1)+0.1),pos.yz);
    //pos.xy = mod(pos.xy+period/2.,period)-period/2.;
    
	//return DE(pos*(abs(sin(iGlobalTime/BOUNCES))*0.3+0.7),ITERATIONS,DETAILS,POWER);  
    return DE(pos*(abs(sin(iGlobalTime/bounces))*0.3+0.7),iterations,details,power);
}


vec3 normals(vec3 pos)
{
    vec2 eps = vec2 (0.01,0.0);
    return normalize(vec3 (SDF(pos+eps.xyy)-SDF(pos-eps.xyy),
                 SDF(pos+eps.yxy)-SDF(pos-eps.yxy),
                 SDF(pos+eps.yyx)-SDF(pos-eps.yyx)));
}

float lighting (vec3 norm)
{
    vec3 lightdir = vec3(0.,1.,0.1);
    return dot(norm,lightdir)*0.5+0.5;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = 2.*(fragCoord.xy / iResolution.xy)-1.;
    uv.x *= iResolution.x/iResolution.y;
    
    vec3 pos = vec3(0.,0.0,-3.5);
    vec3 dir = normalize(vec3(uv*0.4,1.));
    
    vec3 color = vec3(0.0);
    
    for (int i=0; i<70;i++)
    {
    	float d = SDF(pos);
        if (d<0.01) 
        {
            vec3 norm = normals(pos);
            color = vec3(lighting(norm));
            break;
        }
        
        pos += d*dir;
    }
    
	fragColor = vec4(color,1.0);
}