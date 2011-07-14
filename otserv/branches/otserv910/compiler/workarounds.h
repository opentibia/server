//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// STL workarounds
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_WORKAROUNDS_H__
#define __OTSERV_WORKAROUNDS_H__

/*
	Unordered set, unordered map and functional hash workarounds.
	All implementations are inherited to "std" namespace.
*/
#ifdef __OTSERV_FUNCTIONAL_HASH_WORKAROUND__
	namespace std {
		#ifdef _MSC_VER
			template<typename Key, typename Ty = std::less<Key> >
			class hash : public __OTSERV_FUNCTIONAL_HASH_WORKAROUND__<Key, Ty> {};
		#else
			template<typename Key>
			class hash : public __OTSERV_FUNCTIONAL_HASH_WORKAROUND__<Key> {};
		#endif
	}
#endif // __OTSERV_FUNCTIONAL_HASH_WORKAROUND__

#ifdef __OTSERV_UNORDERED_MAP_WORKAROUND__
	namespace std
	{
		#ifdef _MSC_VER
			template<class Key,
					class Ty,
					class Hash = std::hash<Key, std::less<Key> >,
					class Alloc = std::allocator<std::pair<const Key, Ty> > >
					class unordered_map : public __OTSERV_UNORDERED_MAP_WORKAROUND__<Key, Ty, Hash, Alloc> {};
		#else
			template<class Key,
					class Ty,
					class Hash = std::hash<Key>,
					class Pred = std::equal_to<Key>,
					class Alloc = std::allocator<std::pair<const Key, Ty> > >
					class unordered_map : public __OTSERV_UNORDERED_MAP_WORKAROUND__<Key, Ty, Hash, Pred, Alloc> {};
		#endif
	}
#endif // __OTSERV_UNORDERED_MAP_WORKAROUND__

#ifdef __OTSERV_UNORDERED_SET_WORKAROUND__
	namespace std
	{
		#ifdef _MSC_VER
			template<class Key,
					class Hash = std::hash<Key, std::less<Key> >,
					class Alloc = std::allocator<Key> >
					class unordered_set : public __OTSERV_UNORDERED_SET_WORKAROUND__<Key, Hash, Alloc> {};
		#else
			template<class Key,
					class Hash = std::hash<Key>,
					class Pred = std::equal_to<Key>,
					class Alloc = std::allocator<Key> >
					class unordered_set : public __OTSERV_UNORDERED_SET_WORKAROUND__<Key, Hash, Pred, Alloc> {};
		#endif
	}
#endif // __OTSERV_UNORDERED_SET_WORKAROUND__

#endif // __OTSERV_WORKAROUNDS_H__
