/* OpenTibia - an opensource roleplaying game

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __item_h
#define __item_h

#include <list> // stdlibrary list

#include "texcept.h"

/****************************************************************
  Defines the Itemclass
 ****************************************************************/

namespace Items {
	class Item {

		private:
			unsigned itemnumber;
			unsigned short itemcount;

			// list of items if this is a container
			std::list<Item*> lcontained;

			// is this item a container?
			int iscontainer;
			// how many items can be stored maximum?
			int maxitems;
			// and how many do we have already?
			int actualitems;

		public:
			// definition for iterator over backpack items
			typedef std::list<Item*>::const_iterator iterator;

			// return the number of the item
			unsigned  GetItemNumber();

			// get the number of items or 0 if non stackable
			unsigned short  GetItemCount();

			// return if this item is a Container
			int  isContainer();

			// Konstructor for items, needs the itemnumber
			Item(unsigned,unsigned short);

			// Konstructor for containers, the 3rd is the max number of items to store
			Item(unsigned,unsigned short, int);

			// Destructor
			~Item();

			// add an item to the container
			void addItem(Item*);

			// return item iterator
			iterator  getItems();

			// return iterator to one beyond the last element
			iterator  getEnd();

			// put items into the container
			Item& operator<<(Item*);

	}; // class Item

	// now we declare exceptions we throw...
	class nocontainer : public texception {
		public:
			nocontainer() : texception("Item is not a container!", false) {
			} // nocontainer() : texception("Item is not a container!", false) 	
	}; // class nocontainer : public texception 	

	class bad_item : public texception {
		public:
			bad_item() : texception("Item is invalid!", false) {
			} // bad_item() : texception("Item is not a container!", false) 	
	}; // class bad_item : public texception 

	class container_full : public texception {
		public:
			container_full() : texception("container is full!", false) {
			} // container_full() : texception("container is full!", false) 
	}; // class container_full : public texception 

} // namespace Items

#endif
