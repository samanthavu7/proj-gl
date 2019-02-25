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
	    for(int i = 0; i < state.num_vertices; i += 3) { //traverse through every vertex
                const data_geometry** triangle_geometry = new const data_geometry*[3]; //pointer to array of pointers
	       	triangle_geometry[0] = new data_geometry;
		triangle_geometry[1] = new data_geometry;
		triangle_geometry[2] = new data_geometry;
		const_cast<data_geometry*>(triangle_geometry[0])->data = new float[MAX_FLOATS_PER_VERTEX];
		const_cast<data_geometry*>(triangle_geometry[1])->data = new float[MAX_FLOATS_PER_VERTEX];
		const_cast<data_geometry*>(triangle_geometry[2])->data = new float[MAX_FLOATS_PER_VERTEX];

		data_vertex triangle_vertex1;
		data_vertex triangle_vertex2;
		data_vertex triangle_vertex3;
		triangle_vertex1.data = &(state.vertex_data[state.floats_per_vertex * i]);
		triangle_vertex2.data = &(state.vertex_data[state.floats_per_vertex * (i + 1)]);
		triangle_vertex3.data = &(state.vertex_data[state.floats_per_vertex * (i + 2)]);
		for(int j = 0; j < state.floats_per_vertex; j++) {
		    triangle_geometry[0]->data[j] = *triangle_vertex1.data;
		    triangle_geometry[1]->data[j] = *triangle_vertex2.data;
		    triangle_geometry[2]->data[j] = *triangle_vertex3.data;
		}

		state.vertex_shader(triangle_vertex1, const_cast<data_geometry&>(*triangle_geometry[0]), state.uniform_data);
		state.vertex_shader(triangle_vertex2, const_cast<data_geometry&>(*triangle_geometry[1]), state.uniform_data);
		state.vertex_shader(triangle_vertex3, const_cast<data_geometry&>(*triangle_geometry[2]), state.uniform_data);

		rasterize_triangle(state, triangle_geometry);
		
		//delete[] triangle_geometry[0]->data;
		//delete[] triangle_geometry[1]->data;
		//delete[] triangle_geometry[2]->data;
		//delete triangle_geometry[0];
		//delete triangle_geometry[1];
		//delete triangle_geometry[2];
		//delete[] triangle_geometry;
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
