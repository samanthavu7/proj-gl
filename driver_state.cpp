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
    state.image_depth = 0; // initialization - TO DO

    for(int i = 0; i < width * height; i++) {
        state.image_color[i] = make_pixel(0, 0, 0); // initialize to black pixels
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
    switch(type) {
        case render_type::triangle: {
	    const data_geometry *triangle_geometry[3];
	    data_geometry g[state.num_vertices];
	    data_vertex v[state.num_vertices];

	    for(int i = 0, j = 0; i < state.num_vertices * state.floats_per_vertex; i += state.floats_per_vertex, j++) {
		v[j].data = &state.vertex_data[i];
		g[j].data = v[j].data;
	    }

	    for(int i = 0, j = 1, k = 0; i < state.num_vertices; i++, j++, k++) {
		state.vertex_shader(v[i], g[i], state.uniform_data);
		triangle_geometry[k] = &g[i];

		if(!(j % 3) && j) {
		    rasterize_triangle(state, triangle_geometry);
		    k = -1;
		    j = 0;
		}
	    }
	    break;
	}
	case render_type::indexed:
	    break;
	case render_type::fan:
	    break;
	case render_type::strip:
	    break;
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
    if(face==6)
    {
        rasterize_triangle(state, in);
        return;
    }
    std::cout<<"TODO: implement clipping. (The current code passes the triangle through without clipping them.)"<<std::endl;
    clip_triangle(state,in,face+1);
}

// Rasterize the triangle defined by the three vertices in the "in" array.  This
// function is responsible for rasterization, interpolation of data to
// fragments, calling the fragment shader, and z-buffering.
void rasterize_triangle(driver_state& state, const data_geometry* in[3])
{
   data_geometry *out = new data_geometry[3];
//   data_vertex v;
   int i, j, image_index = 0;

   for(int k = 0; k < 3; k++) {	
//	v.data = in[k]->data;
//	state.vertex_shader(v, out[k], state.uniform_data);
	
	out[k].gl_Position[0] /= out[k].gl_Position[3];
	out[k].gl_Position[1] /= out[k].gl_Position[3];

	i = state.image_width / 2.0 * (out[k].gl_Position)[0] + state.image_width / 2.0 - (0.5);
	j = state.image_height / 2.0 * (out[k].gl_Position)[1] + state.image_height / 2.0 - (0.5);

	image_index = i + j * state.image_width;
	state.image_color[image_index] = make_pixel(255, 255, 255);
   }

   int ax = state.image_width / 2.0 * out[0].gl_Position[0] + state.image_width / 2.0 - 0.5;
   int ay = state.image_height / 2.0 * out[0].gl_Position[1] + state.image_height / 2.0 - 0.5;
   int bx = state.image_width / 2.0 * out[1].gl_Position[0] + state.image_width / 2.0 - 0.5;
   int by = state.image_height / 2.0 * out[1].gl_Position[1] + state.image_height / 2.0 - 0.5;
   int cx = state.image_width / 2.0 * out[2].gl_Position[0] + state.image_width / 2.0 - 0.5;
   int cy = state.image_height / 2.0 * out[2].gl_Position[1] + state.image_height / 2.0 - 0.5;

   float area_abc = 0.5 * (((bx * cy) - (cx * by)) - ((ax * cy) - (cx * ay)) - ((ax * by) - (bx * ay)));
   float area_pbc, area_apc, area_abp, alpha, beta, gamma = 0.0;

   for(int x = 0; x < state.image_width; x++) {
       for(int y = 0; y < state.image_height; y++) {
           area_pbc = 0.5 * (((bx * cy) - (cx * by)) + ((by - cy) * x) + ((cx - bx) * y));
	   area_apc = 0.5 * (((ax * cy) - (cx * ay)) + ((cy - ay) * x) + ((ax - cx) * y));
	   area_abp = 0.5 * (((ax * by) - (bx * ay)) + ((ay - by) * x) + ((bx  - ax) * y));

	   alpha = area_pbc / area_abc;
	   beta = area_apc / area_abc;
	   gamma = area_abp / area_abc;

           if(alpha >= 0 && beta >= 0 && gamma >= 0) {
               image_index = x + y * state.image_width;
 	       state.image_color[image_index] = make_pixel(255, 255, 255);
           }
       }
   }  

}
