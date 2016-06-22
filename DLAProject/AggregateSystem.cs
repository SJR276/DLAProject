﻿using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Shapes;

namespace DLAProject {

    /// <summary>
    /// Represents the view / graphics associated with a single diffusion
    /// limited aggregate system.
    /// </summary>
    public class AggregateSystem {
        
        // stack to store particles such that most recently
        // added is accessed in constant time
        private readonly Stack<AggregateParticle> particle_stack;
        private readonly GeometryModel3D particle_model;
        // collections to store graphics related quantities for
        // particle_model MeshGeometry3D Model
        private readonly Point3DCollection particle_positions;
        private readonly Int32Collection triangle_indices;
        private readonly PointCollection tex_coords;
        // radial gradient brush for particle_model material
        private RadialGradientBrush rad_brush;
        // ellipse for target rendering
        private Ellipse ellipse;
        // bitmap for rendering the image source
        private RenderTargetBitmap render_bitmap;

        /// <summary>
        /// Initialises a new instance of the AggregateSystem class.
        /// </summary>
        public AggregateSystem() {
            particle_stack = new Stack<AggregateParticle>();
            particle_model = new GeometryModel3D { Geometry = new MeshGeometry3D() };
            // initialise ellipse with specified Width and Height
            ellipse = new Ellipse {
                Width = 32.0,
                Height = 32.0
            };
            rad_brush = new RadialGradientBrush();
            ellipse.Fill = rad_brush;
            ellipse.Measure(new Size(32, 32));
            ellipse.Arrange(new Rect(0, 0, 32, 32));
            render_bitmap = new RenderTargetBitmap(32, 32, 96, 96, PixelFormats.Pbgra32);
            // create ImageBrush with render_bitmap as ImageSource
            ImageBrush img_brush = new ImageBrush(render_bitmap);
            // create a diffuse material using the image brush
            DiffuseMaterial diff_mat = new DiffuseMaterial(img_brush);
            particle_model.Material = diff_mat;
            particle_positions = new Point3DCollection();
            triangle_indices = new Int32Collection();
            tex_coords = new PointCollection();
            ((MeshGeometry3D)particle_model.Geometry).Positions = particle_positions;
            ((MeshGeometry3D)particle_model.Geometry).TriangleIndices = triangle_indices;
            ((MeshGeometry3D)particle_model.Geometry).TextureCoordinates = tex_coords;
        }

        public Model3D AggregateModel => particle_model;

        /// <summary>
        /// Updates the view of a simulation model for an
        /// aggregate on a 2D lattice.
        /// </summary>
        public void Update() {
            // get the most recently added particle
            AggregateParticle p = particle_stack.Peek();
            int position_index = particle_stack.Count*4;
            // create points according to particle co-ords
            Point3D p1 = new Point3D(p.position.X, p.position.Y, p.position.Z);
            Point3D p2 = new Point3D(p.position.X, p.position.Y + p.size, p.position.Z);
            Point3D p3 = new Point3D(p.position.X + p.size, p.position.Y + p.size, p.position.Z);
            Point3D p4 = new Point3D(p.position.X + p.size, p.position.Y, p.position.Z);
            // add points to particle positions collection
            particle_positions.Add(p1);
            particle_positions.Add(p2);
            particle_positions.Add(p3);
            particle_positions.Add(p4);
            // create points for texture co-ords
            Point t1 = new Point(0.0, 0.0);
            Point t2 = new Point(0.0, 1.0);
            Point t3 = new Point(1.0, 1.0);
            Point t4 = new Point(1.0, 0.0);
            // add texture co-ords points to texcoords collection
            tex_coords.Add(t1);
            tex_coords.Add(t2);
            tex_coords.Add(t3);
            tex_coords.Add(t4);
            // add position indices to indices collection
            triangle_indices.Add(position_index);
            triangle_indices.Add(position_index + 2);
            triangle_indices.Add(position_index + 1);
            triangle_indices.Add(position_index);
            triangle_indices.Add(position_index + 3);
            triangle_indices.Add(position_index + 2);
            // set particle_model Geometry model properties 
            rad_brush.GradientStops.Add(new GradientStop(p.colour, 0.0));
            render_bitmap.Render(ellipse); 
        }

        /// <summary>
        /// Updates the view of a simulation model for an 
        /// aggregate on a 3D lattice.
        /// </summary>
        public void Update3D() {
            // TODO: apply simulation view updates creating spheres to add to particle_model
        }

        /// <summary>
        /// Clears the view of the aggregate simulation.
        /// </summary>
        public void Clear() {
            // clear all collections
            particle_stack.Clear();
            ((MeshGeometry3D)particle_model.Geometry).Positions.Clear();
            ((MeshGeometry3D)particle_model.Geometry).TriangleIndices.Clear();
            ((MeshGeometry3D)particle_model.Geometry).TextureCoordinates.Clear();
            rad_brush = new RadialGradientBrush();
            ellipse.Fill = rad_brush;
        }

        /// <summary>
        /// Spawns a new AggregateParticle with specified properties.
        /// </summary>
        /// <param name="_position">Position in 3D space of particle.</param>
        /// <param name="_colour">Colour of particle.</param>
        /// <param name="_size">Size of particle.</param>
        public void SpawnParticle(Point3D _position, Color _colour, double _size) {
            AggregateParticle agg_particle = new AggregateParticle {
                position = _position, colour = _colour, size = _size
            };
            // add agg_particle to particle_stack container
            particle_stack.Push(agg_particle);
        }

    }

}


