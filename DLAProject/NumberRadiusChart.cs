﻿using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using LiveCharts;
using LiveCharts.Wpf;
using LiveCharts.Configurations;

namespace DLAProject {
    /// <summary>
    /// Measurement model used for charting the radius of an aggregate against its particle number.
    /// </summary>
    public class NumberRadiusMeasureModel {
        public uint ParticleNumber { get; set; }
        public double AggregateRadius { get; set; }
    }

    /// <summary>
    /// Represents a chart which plots the radius of a diffusion limited aggregate against 
    /// the number of particles in the system.
    /// </summary>
    public class NumberRadiusChart : INotifyPropertyChanged {
        private uint x_axis_min;    // minimum value of x-axis
        private uint x_axis_max;    // maximum value of x-axis
        private uint x_axis_step;

        /// <summary>
        /// Initialises a new instance of the NumberRadiusChart class.
        /// </summary>
        public NumberRadiusChart() {
            // create a mapper using NumberRadiusMeasureModel where
            // X co-ord is ParticleNumber and Y co-ord is Radius.
            var mapper = Mappers.Xy<NumberRadiusMeasureModel>()
                 .X(model => model.ParticleNumber)
                 .Y(model => model.AggregateRadius);
            // save the mapper globally
            Charting.For<NumberRadiusMeasureModel>(mapper);
            // initialise the ChartValues instance
            Values = new ChartValues<NumberRadiusMeasureModel>();
            SetAxisLimits(0);
            x_axis_step = 100;
        }

        public event PropertyChangedEventHandler PropertyChanged;
        /// <summary>
        /// Handle property changed event.
        /// </summary>
        /// <param name="propertyName">Name of property that was changed.</param>
        protected virtual void OnPropertyChanged(string propertyName = null) {
            if (PropertyChanged != null)
                PropertyChanged.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public ChartValues<NumberRadiusMeasureModel> Values { get; set; }

        /// <summary>
        /// Incremental step of x-axis.
        /// </summary>
        public uint AxisStep {
            get { return x_axis_step; }
        }

        /// <summary>
        /// Sets the step increment of the chart x-axis based on number of particles to generate.
        /// </summary>
        /// <param name="_total_particles">Number of particles to be generated by aggregate.</param>
        public void SetXAxisStep(uint _total_particles) {
            if (_total_particles < 2500)
                x_axis_step = 100;
            else if (_total_particles >= 2500 && _total_particles < 5000)
                x_axis_step = 200;
            else if (_total_particles >= 5000 && _total_particles < 7500)
                x_axis_step = 300;
            else
                x_axis_step = 400;
            // fire PropertyChanged even on AxisStep field
            OnPropertyChanged("AxisStep");
        }

        /// <summary>
        /// Minimum value of x-axis.
        /// </summary>
        public uint AxisMin {
            get { return x_axis_min; }
            set { x_axis_min = value; OnPropertyChanged("AxisMin"); }
        }
        /// <summary>
        /// Maximum value of x-axis.
        /// </summary>
        public uint AxisMax {
            get { return x_axis_max; }
            set { x_axis_max = value; OnPropertyChanged("AxisMax"); }
        }
        /// <summary>
        /// Set the limits of the x-axis, minimum will always be zero whilst maximum
        /// will take the value of the parameter _total_particles.
        /// </summary>
        /// <param name="_total_particles">Maximum value of x-axis.</param>
        public void SetAxisLimits(uint _total_particles) {
            AxisMin = 0;
            AxisMax = _total_particles;
        }
        /// <summary>
        /// Adds a data point to the chart with specified x,y values.
        /// </summary>
        /// <param name="_particles">Number of particles (x co-ordinate).</param>
        /// <param name="_radius">Radius of aggregate (y co-ordinate).</param>
        public void AddDataPoint(uint _particles, double _radius) {
            Values.Add(new NumberRadiusMeasureModel {
                ParticleNumber = _particles,
                AggregateRadius = _radius
            });
        }
        /// <summary>
        /// Clears all data points from the chart.
        /// </summary>
        public void ClearAllDataPoints() {
            Values.Clear();
        }

    }
}
