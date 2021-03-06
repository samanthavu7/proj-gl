#include "driver_state.h"
#include <cstring>

driver_state::driver_state()
{
}

driver_state::~driver_state()
{
    delete [] image_color;
    delete [] image_depth;
}

// This function should allocate and initialize the arrays that store color and
// depth.  This is not done during the constructor since the width and height
// are not known when this class is constructed.
void initialize_render(driver_state& state, int width, int height)
{
    state.image_width = width;
    state.image_height = height;
    state.image_color = new pixel[width * height];
    state.image_depth = new float[width * height]; 

    for(int i = 0; i < width * height; i++) {
        state.image_color[i] = make_pixel(0, 0, 0); 
	state.image_depth[i] = 1;
    }
}

// This function will be called to render the data that has been stored in this class.
// Valid values of type are:
//   render_type::triangle - Each group of three vertices corresponds to a triangle.
//   render_type::indexed -  Each group of three indices in index_data corresponds
//                           to a triangle.  These numbers are indices into vertex_data.
//   render_type::fan -      The vertices are to be interpreted as a triangle fan.
//   render_type::strip -    The vertices are to be interpreted as a triangle strip.
void render(driver_state& state, render_type type)
{
    const data_geometry* out[3];
    data_geometry g[3];
    data_vertex v[3];

    switch(type) {
        case render_type::triangle: {
	   int beg = 0;
	   for(int i = 0; i < state.num_vertices; i += 3) {
		for(int j = 0; j < 3; j++) {			
		    v[j].data = &state.vertex_data[beg];
		    g[j].data = v[j].data;
		    state.vertex_shader(v[j], g[j], state.uniform_data);
		    out[j] = &g[j];
		    beg += state.floats_per_vertex;
		}
		clip_triangle(state, out, 0);
	    }
	    break;
	}
	case render_type::indexed: {
	    for(int i = 0; i < 3 * state.num_triangles; i += 3) {
		for(int j = 0; j < 3; j++) {
		    v[j].data = &state.vertex_data[state.index_data[i + j] * state.floats_per_vertex];
		    g[j].data = v[j].data;
		    state.vertex_shader(v[j], g[j], state.uniform_data);
		    out[j] = &g[j];
		}
		clip_triangle(state, out, 0);
	    }
	    break;
	}
	case render_type::fan: {
	    for(int i = 0; i < state.num_vertices; i++) {
		for(int j = 0; j < 3; j++) {
		    int index = i + j;
		    if(j == 0) { index = 0; }
		    v[j].data = &state.vertex_data[index * state.floats_per_vertex];
		    g[j].data = v[j].data;
		    state.vertex_shader(v[j], g[j], state.uniform_data);
		    out[j] = &g[j];
		}
		clip_triangle(state, out, 0);
	    }
	    break;
	}
	case render_type::strip: {
	    for(int i = 0; i < state.num_vertices - 2; i++) {
		for(int j = 0; j < 3; j++) {
		    v[j].data = &state.vertex_data[(i + j) * state.floats_per_vertex];
		    g[j].data = v[j].data;
		    state.vertex_shader(v[j], g[j], state.uniform_data);
		    out[j] = & g[j];
		}
		clip_triangle(state, out, 0);
	    }
	    break;
	}
	default:
	    break;
    }    

}


// This function clips a triangle (defined by the three vertices in the "in" array).
// It will be called recursively, once for each clipping face (face=0, 1, ..., 5) to
// clip against each of the clipping faces in turn.  When face=6, clip_triangle should
// simply pass the call on to rasterize_triangle.
void clip_triangle(driver_state& state, const data_geometry* in[3],int face)
{
    if(face == 1)
    {
        rasterize_triangle(state, in);
        return;
    }
    
    vec4 a = in[0]->gl_Position;
    vec4 b = in[1]->gl_Position;
    vec4 c = in[2]->gl_Position;
    const data_geometry *input[3] = { in[0], in[1], in[2] };
    data_geometry new_vertex0[3];
    data_geometry new_vertex1[3];
    float alpha0, alpha1, alpha_prime;
    vec4 p0, p1;

    if(a[2] <= -a[3] && b[2] <= -b[3] && c[2] <= -c[3]) { return; }
    else 
	if(a[2] <= -a[3] && b[2] > -b[3] && c[2] > -c[3]) {
	    alpha0 = (-b[3] - b[2]) / (a[2] + a[3] - b[3] - b[2]);
	    alpha1 = (-a[3] - a[2]) / (c[2] + c[3] - a[3] - a[2]);
	    p0 = alpha0 * a + (1 - alpha0) * b;
	    p1 = alpha1 * c + (1 - alpha1) * a;

	    new_vertex0[0].data = new float[state.floats_per_vertex];
	    new_vertex0[1] = *in[1];
	    new_vertex0[2] = *in[2];

	    for(int i = 0; i < state.floats_per_vertex; i++) {
		switch(state.interp_rules[i]) {
		    case interp_type::flat:
			new_vertex0[0].data[i] = in[0]->data[i];
			break;
		    case interp_type::smooth:
			new_vertex0[0].data[i] = alpha1 * in[2]->data[i] + (1 - alpha1) * in[0]->data[i];
			break;
		    case interp_type::noperspective:
			alpha_prime = alpha1 * in[2]->gl_Position[3] / (alpha1 * in[2]->gl_Position[3] + (1 - alpha1) * in[0]->gl_Position[3]);
			new_vertex0[0].data[i] = alpha_prime * in[2]->data[i] + (1 - alpha_prime) * in[0]->data[i];
			break;
		    default:
			break;
		}
	    }
 
	    new_vertex0[0].gl_Position = p1;
	    input[0] = &new_vertex0[0];
	    input[1] = &new_vertex0[1];
	    input[2] = &new_vertex0[2];

	    clip_triangle(state, input, face + 1);

	    new_vertex1[0].data = new float[state.floats_per_vertex];

	    for(int i = 0; i < state.floats_per_vertex; i++) 
		switch(state.interp_rules[i]) {
		    case interp_type::flat:
			new_vertex1[0].data[i] = in[0]->data[i];
			break;
		    case interp_type::smooth:
			new_vertex1[0].data[i] = alpha0 * in[0]->data[i] + (1 - alpha0) * in[1]->data[i];
			break;
		    case interp_type::noperspective:
			alpha_prime = alpha0 * in[0]->gl_Position[3] / (alpha0 * in[0]->gl_Position[3] * (1 - alpha0) * in[1]->gl_Position[3]);
			new_vertex1[0].data[i] = alpha_prime * in[0]->data[i] + (1 - alpha_prime) * in[1]->data[i];
			break;
		    default:
			break;
		}
	    
	    new_vertex1[0].gl_Position = p0;
	    input[0] = &new_vertex1[0];
	    input[1] = &new_vertex0[1];
	    input[2] = &new_vertex0[0];
	
	}
	
	clip_triangle(state, input, face + 1);

}

// Rasterize the triangle defined by the three vertices in the "in" array.  This
// function is responsible for rasterization, interpolation of data to
// fragments, calling the fragment shader, and z-buffering.
void rasterize_triangle(driver_state& state, const data_geometry* in[3])
{
   // convert to NDC coordinates (i,j)
   float i[3], j[3], k[3];
   for(int n = 0; n < 3; n++) {	
	i[n] = state.image_width / 2.0 * (in[n]->gl_Position[0] / in[n]->gl_Position[3]) + state.image_width / 2.0 - 0.5;
	j[n] = state.image_height / 2.0 * (in[n]->gl_Position[1] / in[n]->gl_Position[3]) + state.image_height / 2.0 - 0.5;
	k[n] = in[n]->gl_Position[2] / in[n]->gl_Position[3];
   }

   float min_i = std::min(std::min(i[0],i[1]),i[2]);
   float max_i = std::max(std::max(i[0],i[1]),i[2]);
   float min_j = std::min(std::min(j[0],j[1]),j[2]);
   float max_j = std::max(std::max(j[0],j[1]),j[2]);

   if(min_i < 0) { min_i = 0; } 
   if(min_j < 0) { min_j = 0; }
   if(max_i > state.image_width) { max_i = state.image_width; }
   if(max_j > state.image_height) { max_j = state.image_height; }

   float area = 0.5 * ((i[1] * j[2] - i[2] * j[1]) - (i[0] * j[2] - i[2] * j[0]) + (i[0] * j[1] - i[1] * j[0]));
   float alpha, beta, gamma = 0.0;
 
   float* data = new float[MAX_FLOATS_PER_VERTEX];
   data_fragment frag{data};
   data_output out;
   float temp, depth, alpha_prime, beta_prime, gamma_prime = 0.0;
  
   for(int x = min_i; x < max_i; x++) {
       for(int y = min_j; y < max_j; y++) {
           alpha = 0.5 * ((i[1] * j[2] - i[2] * j[1]) + (j[1] - j[2]) * x + (i[2] - i[1]) * y) / area;
	   beta = 0.5 * ((i[2] * j[0] - i[0] * j[2]) + (j[2] - j[0]) * x + (i[0] - i[2]) * y) / area;
	   gamma = 0.5 * ((i[0] * j[1] - i[1] * j[0]) + (j[0] - j[1]) * x + (i[1] - i[0]) * y) / area;

           if(alpha >= 0 && beta >= 0 && gamma >= 0) {      
	       //depth = alpha * (in[0]->gl_Position[2] / in[0]->gl_Position[3]) + beta * (in[1]->gl_Position[2] / in[1]->gl_Position[3]) + gamma * (in[2]->gl_Position[2] / in[2]->gl_Position[3]);
	       depth = alpha * k[0] + beta * k[1] + gamma * k[2];
	       if(state.image_depth[x + y * state.image_width] > depth) {
	           for(int z = 0; z < state.floats_per_vertex; z++) {
	               switch(state.interp_rules[z]) {
	                   case interp_type::flat:
		               frag.data[z] = in[0]->data[z];
		               break;
		           case interp_type::smooth:
		               temp = alpha / in[0]->gl_Position[3] + beta / in[1]->gl_Position[3] + gamma / in[2]->gl_Position[3]; 
			       alpha_prime = alpha / (temp * in[0]->gl_Position[3]);
			       beta_prime = beta / (temp * in[1]->gl_Position[3]);
			       gamma_prime = gamma / (temp * in[2]->gl_Position[3]);
			       frag.data[z] = alpha_prime * in[0]->data[z] + beta_prime * in[1]->data[z] + gamma_prime * in[2]->data[z];
			       break;
		           case interp_type::noperspective:
			       frag.data[z] = alpha * in[0]->data[z] + beta * in[1]->data[z] + gamma * in[2]->data[z];
		               break;
		           default:
		               break;
	               }
	           }

	           state.fragment_shader(frag, out, state.uniform_data);
	           state.image_color[x + y * state.image_width] = make_pixel(out.output_color[0] * 255, out.output_color[1] * 255, out.output_color[2] * 255);
		    state.image_depth[x + y * state.image_width] = depth;
	       }
	   }   
       }
   }  
}
