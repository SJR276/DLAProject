﻿using System;
using System.ComponentModel;
using System.Windows.Media;
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
    /// the number of particles in the system. Inherits from INotifyPropertyChanged in order
    /// to notify client when a chart property changed event has been fired.
    /// </summary>
    public class NumberRadiusChart : INotifyPropertyChanged {
        private uint x_axis_min;    // minimum value of x-axis
        private uint x_axis_max;    // maximum value of x-axis
        private uint x_axis_step;   // incremental step of x-axis
        private int series_counter;

        /// <summary>
        /// Initialises a new instance of the NumberRadiusChart class.
        /// </summary>
        public NumberRadiusChart() {
            // create a mapper using NumberRadiusMeasureModel where
            // X co-ord is ParticleNumber and Y co-ord is Radius.
            CartesianMapper<NumberRadiusMeasureModel> mapper = Mappers.Xy<NumberRadiusMeasureModel>()
                 .X(model => model.ParticleNumber)
                 .Y(model => model.AggregateRadius);
            // save the mapper globally
            Charting.For<NumberRadiusMeasureModel>(mapper);
            NRSeriesCollection = new SeriesCollection();
            series_counter = -1;
            // initialise the ChartValues instance
            //Values = new ChartValues<NumberRadiusMeasureModel>();
            //ValuesSecond = new ChartValues<NumberRadiusMeasureModel>();
            ResetXAxisProperties();
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

        public SeriesCollection NRSeriesCollection { get; set; }

        /// <summary>
        /// Incremental step of x-axis.
        /// </summary>
        public uint AxisStep {
            get { return x_axis_step; }
            set { x_axis_step = value; OnPropertyChanged("AxisStep"); }
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
        public void ResetXAxisProperties() {
            AxisMin = 0;
            AxisMax = 2000;
            AxisStep = 100;
        }
        public void AddDataSeries(uint nparticles, double coeff_stick) {
            NRSeriesCollection.Add(new LineSeries {
                Title = nparticles + ", " + coeff_stick,
                Values = new ChartValues<NumberRadiusMeasureModel>(),
                PointGeometrySize = 5,
                Fill = Brushes.Transparent
            });
            ++series_counter;
        }

        /// <summary>
        /// Adds a data point to the chart with specified x,y values.
        /// </summary>
        /// <param name="_particles">Number of particles (x co-ordinate).</param>
        /// <param name="_radius">Radius of aggregate (y co-ordinate).</param>
        public void AddDataPoint(uint _particles, double _radius) {
            NRSeriesCollection[series_counter].Values.Add(new NumberRadiusMeasureModel {
                ParticleNumber = _particles,
                AggregateRadius = _radius
            });
            OnPropertyChanged("AxisMin");
        }
        /// <summary>
        /// Clears all data points from the chart.
        /// </summary>
        public void ClearAllSeriesDataPoints() {
            foreach (var series in NRSeriesCollection) {
                series.Values.Clear();
            }
            series_counter = -1;
        }

    }
}