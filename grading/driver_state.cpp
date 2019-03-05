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
                data_geometry** triangle_geometry = new data_geometry*[3]; //pointer to array of pointers
	       	
		for(int j = 0; j < 3; j++) {
		    triangle_geometry[j] = new data_geometry;
		    data_vertex triangle_vertex;
		    triangle_vertex.data = new float[MAX_FLOATS_PER_VERTEX];
		    triangle_geometry[j]->data = new float[MAX_FLOATS_PER_VERTEX];
		 		
		    for(int k = 0; k < state.floats_per_vertex; k++) {
		    	triangle_vertex.data[k] = state.vertex_data[k + state.floats_per_vertex * (i + j)];
		    	triangle_geometry[j]->data[k] = triangle_vertex.data[k];
		    }

		    state.vertex_shader(triangle_vertex, *triangle_geometry[j], state.uniform_data);
		}

		rasterize_triangle(state, (const data_geometry**)triangle_geometry);
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
   // convert to NDC coordinates (i,j)
   int pixels[3][2]; //int i[3], j[3];
   for(int k = 0; k < 3; k++) {	
	pixels[k][0] = state.image_width / 2.0 * (in[k]->gl_Position[0] / in[k]->gl_Position[3]) + state.image_width / 2.0 - 0.5;
	pixels[k][1] = state.image_height / 2.0 * (in[k]->gl_Position[1] / in[k]->gl_Position[3]) + state.image_height / 2.0 - 0.5;
   }

   //int min_i = std::min(std::min(i[0],i[1]),i[2]);
   //int max_i = std::max(std::max(i[0],i[1]),i[2]);
   //int min_j = std::min(std::min(j[0],j[1]),j[2]);
   //int max_j = std::max(std::max(j[0],j[1]),j[2]);

   //if(min_i < 0) { min_i = 0; } //check
   //if(min_j < 0) { min_j = 0; }
   //if(max_i > state.image_width) { max_i = state.image_width - 1; }
   //if(max_j > state.image_height) { max_j = state.image_height - 1; }

   int ax = pixels[0][0]; int ay = pixels[0][1];
   int bx = pixels[1][0]; int by = pixels[1][1];
   int cx = pixels[2][0]; int cy = pixels[2][1];

   double area = 0.5 * ((bx*cy - cx*by) - (ax*cy - cx*ay) + (ax*by - bx*ay));//0.5 * ((i[1] * j[2] - i[2] * j[1]) - (i[0] * j[2] - i[2] * j[0]) - (i[0] * j[1] - i[1] * j[0]));
   double alpha, beta, gamma = 0.0;
 
   float* data = new float[MAX_FLOATS_PER_VERTEX];
   data_fragment frag{data};
   data_output out;
   float temp, ap, bp, gp;
  
   for(int y = 0; y < state.image_height; y++) {
       for(int x = 0; x < state.image_width; x++) {
           alpha = 0.5 * ((bx*cy - cx*by) + (by-cy)*x + (cx-bx)*y) / area; //0.5 * ((i[1] * j[2] - i[2] * j[1]) + (j[1] - j[2]) * x + (i[2] - i[1]) * y) / area;
	   beta = 0.5 * ((cx*ay - ax*cy) + (cy-ay)*x + (ax-cx)*y) / area;//0.5 * ((i[2] * j[0] - i[0] * j[2]) + (j[2] - j[0]) * x + (i[0] - i[2]) * y) / area;
	   gamma = 0.5 * ((ax*by - bx * ay) + (ay-by)*x + (bx-ax)*y) / area;//0.5 * ((i[0] * j[1] - i[1] * j[0]) + (j[0] - j[1]) * x + (i[1] - i[0]) * y) / area;

           if(alpha >= 0 && beta >= 0 && gamma >= 0) {      
	       for(int z = 0; z < state.floats_per_vertex; z++) {
	           switch(state.interp_rules[z]) {
	               case interp_type::flat:
		           frag.data[z] = in[0]->data[z];
		           break;
		       case interp_type::smooth:
		           temp = alpha / in[0]->gl_Position[3] + beta / in[1]->gl_Position[3] + gamma / in[2]->gl_Position[3]; 
			   ap = alpha / (temp * in[0]->gl_Position[3]);
			   bp = beta / (temp * in[1]->gl_Position[3]);
			   gp = gamma / (temp * in[2]->gl_Position[3]);
			   frag.data[z] = ap * in[0]->data[z] + bp * in[1]->data[z] + gp * in[2]->data[z];
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
	   }   
       }
   }  
}
