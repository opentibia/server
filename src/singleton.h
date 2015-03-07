//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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

#ifndef __OTSERV_SINGLETON_H__
#define __OTSERV_SINGLETON_H__

#include <boost/noncopyable.hpp>
#include <boost/thread/once.hpp>
#include <boost/scoped_ptr.hpp>

/// The Singleton class is a simple class that can be used to initialize once a
/// resource. The resource being hold in it is initialized only when
/// "getInstance()" is called for the first time, avoiding initialization fiasco.
template<typename T>
class Singleton : boost::noncopyable
{
  /// Initialize a resource of type T
  static void initialize()
  {
    m_pointer.reset(new T);
  }

public:
  /// Initialize the internal instance if still not initialized
  /// and returns it.
  ///
  /// @return A pointer to the current instance.
  ///
  /// @throw Any exception the resource can throw during construction
  /// or any exception during calling the "new" operator.
  static T* get()
  {
    boost::call_once(m_flag, initialize);
    return m_pointer.get();
  }

private:
  /// A scoped pointer holding the actual resource.
  static boost::scoped_ptr<T> m_pointer;

  /// One-time initialization flag
  static boost::once_flag m_flag;
};

template<typename T>
boost::scoped_ptr<T> Singleton<T>::m_pointer;

template<typename T>
boost::once_flag Singleton<T>::m_flag = BOOST_ONCE_INIT;

#endif // __OTSERV_SINGLETON_H__
