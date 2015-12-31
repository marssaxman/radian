// Copyright 2009-2012 Mars Saxman.
//
// Radian is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 2 of the License, or (at your option) any later
// version.
//
// Radian is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// Radian.  If not, see <http://www.gnu.org/licenses/>.


#ifndef sequence_h
#define sequence_h

template <typename T>
class Iterator
{
	public:
		virtual ~Iterator<T>() {}
		virtual bool Next() = 0;
		virtual T Current() const = 0;
};

template <typename T>
class Sequence
{
	public:
		virtual ~Sequence<T>() {}
		virtual Iterator<T> Iterate() = 0;
};

#endif //sequence_h
