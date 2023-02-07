﻿using Client.Domain.Common;
using Client.Domain.Entities;
using Client.Domain.Enums;
using Client.Domain.ValueObjects;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Client.Application.ViewModels
{
    public class CreatureMapViewModel : ObservableObject
    {
        public uint Id => creature.Id;
        public string Name => creature.Name;
        public Vector3 Position => new Vector3(
            (creature.Transform.Position.X - hero.Transform.Position.X) / scale + (VieportSize.X / 2),
            (creature.Transform.Position.Y - hero.Transform.Position.Y) / scale + (VieportSize.Y / 2),
            0
        );
        public Vector3 Direction => new Vector3(
            creature.Transform.Direction.X * Radius * 2f,
            creature.Transform.Direction.Y * Radius * 2f,
            0
        );
        public VitalStats VitalStats => creature.VitalStats;
        public float Radius => MathF.Max(MAX_RADIUS / scale, MIN_RADIUS);
        public float Scale
        {
            get => scale;
            set
            {
                if (scale != value)
                {
                    scale = value;
                    OnPropertyChanged("Position");
                    OnPropertyChanged("Direction");
                    OnPropertyChanged("Radius");
                    OnPropertyChanged("AggroRadius");
                }
            }
        }
        public Vector3 VieportSize
        {
            get => vieportSize;
            set
            {
                if (vieportSize != value)
                {
                    vieportSize = value;
                    OnPropertyChanged("Position");
                    OnPropertyChanged("Direction");
                }
            }
        }
        public CreatureTypeEnum Type => creature.Type;
        public bool IsTarget => Id == hero.TargetId;
        public bool IsAggressive => creature.AggroRadius > 0;
        public float AggroRadius => creature.AggroRadius / scale;


        public CreatureMapViewModel(CreatureInterface creature, Hero hero)
        {
            this.creature = creature;
            this.hero = hero;
            creature.PropertyChanged += Creature_PropertyChanged;
            creature.Transform.PropertyChanged += Transform_PropertyChanged;
            creature.Transform.Position.PropertyChanged += Position_PropertyChanged;
            creature.VitalStats.PropertyChanged += VitalStats_PropertyChanged;
            hero.Transform.Position.PropertyChanged += HeroPosition_PropertyChanged;
            hero.PropertyChanged += Hero_PropertyChanged;
        }

        private void VitalStats_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "Hp" || e.PropertyName == "MaxHp")
            {
                OnPropertyChanged("VitalStats");
            }
        }

        private void Hero_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "TargetId")
            {
                OnPropertyChanged("IsTarget");
            }
        }

        private void HeroPosition_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            OnPropertyChanged("Position");
            OnPropertyChanged("Direction");
        }

        private void Position_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            OnPropertyChanged("Position");
            OnPropertyChanged("Direction");
        }

        private void Transform_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "Direction")
            {
                OnPropertyChanged("Direction");
            }
        }

        private void Creature_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "Name")
            {
                OnPropertyChanged("Name");
            }
        }

        private readonly CreatureInterface creature;
        private readonly Hero hero;
        private float scale = 1;
        private static readonly float MAX_RADIUS = 10;
        private static readonly float MIN_RADIUS = 4;
        private Vector3 vieportSize = new Vector3(0, 0, 0);
    }
}
