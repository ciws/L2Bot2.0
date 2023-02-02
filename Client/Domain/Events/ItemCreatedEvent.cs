﻿using Client.Domain.Entities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Client.Domain.Events
{
    public class ItemCreatedEvent : EventInterface
    {
        public readonly BaseItem Item;

        public ItemCreatedEvent(BaseItem item)
        {
            Item = item;
        }
    }
}
