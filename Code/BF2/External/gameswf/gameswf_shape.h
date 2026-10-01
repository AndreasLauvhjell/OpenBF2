// gameswf_shape.h	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Quadratic bezier outline shapes, the basis for most SWF rendering.


#ifndef GAMESWF_SHAPE_H
#define GAMESWF_SHAPE_H


#include "gameswf_styles.h"


namespace gameswf
{
	struct character;
	struct stream;
	struct shape_character_def;
	namespace tesselate {
		struct trapezoid_accepter;
		struct tesselating_shape {
			virtual void tesselate(float error_tolerance, 
					       trapezoid_accepter *accepter) const = 0;
		};
	}


	struct edge
	// Together with the previous anchor, defines a quadratic
	// curve segment.
	{
		edge();
		edge(float cx, float cy, float ax, float ay);
		void	tesselate_curve() const;
		bool	is_straight() const;
		
	//private:
		// *quadratic* bezier: point = p0 * t^2 + p1 * 2t(1-t) + p2 * (1-t)^2
		float	m_cx, m_cy;		// "control" point
		float	m_ax, m_ay;		// "anchor" point
	};


	struct path
	// A subset of a shape -- a series of edges sharing a single set
	// of styles.
	{
		path();
		path(float ax, float ay, int fill0, int fill1, int line);

		void	reset(float ax, float ay, int fill0, int fill1, int line);
		bool	is_empty() const;

		bool	point_test(float x, float y);

		// Push the path into the tesselator.
		void	tesselate() const;

	//private:
		int	m_fill0, m_fill1, m_line;
		float	m_ax, m_ay;	// starting point
		array<edge>	m_edges;
		bool	m_new_shape;
	};

	struct mesh
	// For holding a pre-tesselated shape.
	{
		mesh();

		void	set_tri_strip(const point pts[], int count);

		void	display(const base_fill_style& style, float ratio) const;

		void	output_cached_data(tu_file* out);
		void	input_cached_data(tu_file* in);
	private:
		array<Sint16>	m_triangle_strip;
	};


	struct line_strip
	// For holding a line-strip (i.e. polyline).
	{
		line_strip();
		line_strip(int style, const point coords[], int coord_count);

		void	display(const base_line_style& style, float ratio) const;

		int	get_style() const { return m_style; }
		void	output_cached_data(tu_file* out);
		void	input_cached_data(tu_file* in);
	private:
		int	m_style;
		array<Sint16>	m_coords;
	};


	struct mesh_set
	// A whole shape, tesselated to a certain error tolerance.
	{
		mesh_set();
		mesh_set(const tesselate::tesselating_shape* sh,
			 float error_tolerance);

//		int	get_last_frame_rendered() const;
//		void	set_last_frame_rendered(int frame_counter);
		float	get_error_tolerance() const { return m_error_tolerance; }

		void display(
			const matrix& m,
			const cxform& cx,
			const array<fill_style>& fills,
			const array<line_style>& line_styles) const;

		void display(
			const matrix& m,
			const cxform& cx,
			const array<morph_fill_style>& fills,
			const array<morph_line_style>& line_styles,
			float ratio) const;

		void	set_tri_strip(int style, const point pts[], int count);
		void	add_line_strip(int style, const point coords[], int coord_count);

		void	output_cached_data(tu_file* out);
		void	input_cached_data(tu_file* in);

	private:
//		int	m_last_frame_rendered;	// @@ Hm, we shouldn't spontaneously drop cached data I don't think...
		float	m_error_tolerance;
		array<mesh>	m_meshes;	// One mesh per style.
		array<line_strip>	m_line_strips;
	};


	struct shape_character_def : public character_def, public tesselate::tesselating_shape
	// Represents the outline of one or more shapes, along with
	// information on fill and line styles.
	{
		shape_character_def();
		virtual ~shape_character_def();

		virtual void	display(character* inst);
		bool	point_test_local(float x, float y);

		float	get_height_local();
		float	get_width_local();

		void	read(stream* in, int tag_type, bool with_style, movie_definition_sub* m);
		void	display(
			const matrix& mat,
			const cxform& cx,
			float pixel_scale,
			const array<fill_style>& fill_styles,
			const array<line_style>& line_styles) const;
		virtual void	tesselate(float error_tolerance, tesselate::trapezoid_accepter* accepter) const;
		const rect&	get_bound() const { return m_bound; }
		void	compute_bound(rect* r) const;	// @@ what's the difference between this and get_bound?

		void	output_cached_data(tu_file* out, const cache_options& options);
		void	input_cached_data(tu_file* in);

		const array<fill_style>&	get_fill_styles() const { return m_fill_styles; }
		const array<line_style>&	get_line_styles() const { return m_line_styles; }
		const array<path>&	get_paths() const { return m_paths; }

		// morph uses this
		void	set_bound(const rect& r) { m_bound = r; /* should do some verifying */ }

		// [DICE-MOD SwiffPlayer: the mutation surface the drawing API and the loadMovie image
		// loader need on a runtime-built shape (the shipped canvas is a `_____defaultShape` child
		// whose definition gameswf_sprite_line_style/move_to/line_to/begin_fill/end_fill mutate:
		// styles appended and referenced 1-based, paths appended, the bound recomputed after every
		// edge). The tesselation cache is thrown away on every change so display() re-tesselates.]
		int	add_fill_style(const fill_style& f) { m_fill_styles.push_back(f); clear_cached_meshes(); return m_fill_styles.size(); }
		int	add_line_style(const line_style& l) { m_line_styles.push_back(l); clear_cached_meshes(); return m_line_styles.size(); }
		path*	current_path() { return m_paths.size() > 0 ? &m_paths[m_paths.size() - 1] : NULL; }
		path*	add_path(const path& p) { m_paths.push_back(p); clear_cached_meshes(); return &m_paths[m_paths.size() - 1]; }
		void	refresh_bound() { compute_bound(&m_bound); clear_cached_meshes(); }
		void	clear_cached_meshes()
		{
			for (int i = 0; i < m_cached_meshes.size(); i++)
			{
				delete m_cached_meshes[i];
			}
			m_cached_meshes.resize(0);
		}

	protected:
		friend struct morph2_character_def;

		// derived morph classes changes these
		array<fill_style>	m_fill_styles;
		array<line_style>	m_line_styles;
		array<path>	m_paths;

	private:
		void	sort_and_clean_meshes() const;
		
		rect	m_bound;

		// Cached pre-tesselated meshes.
		mutable array<mesh_set*>	m_cached_meshes;
	};

}	// end namespace gameswf


#endif // GAMESWF_SHAPE_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
