/* Copyright 2021 Franz Poeschel
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "openPMD/ReadIterations.hpp"

#include "openPMD/Series.hpp"

namespace openPMD
{

SeriesIterator::SeriesIterator() : m_series()
{
}

SeriesIterator::SeriesIterator( Series series )
    : m_series( std::move( series ) )
{
    auto it = series.get().iterations.begin();
    if( it == series.get().iterations.end() )
    {
        *this = end();
        return;
    }
    else
    {
        auto status = it->second.beginStep();
        if( status == AdvanceStatus::OVER )
        {
            *this = end();
            return;
        }
        it->second.setStepStatus( StepStatus::DuringStep );
    }
    m_currentIteration = it->first;
}

SeriesIterator & SeriesIterator::operator++()
{
    if( !m_series.has_value() )
    {
        *this = end();
        return *this;
    }
    Series & series = m_series.get();
    auto & iterations = series.iterations;
    auto & currentIteration = iterations[ m_currentIteration ];
    if( !currentIteration.closed() )
    {
        currentIteration.close();
    }
    switch( series.iterationEncoding() )
    {
        using IE = IterationEncoding;
    case IE::groupBased: {
        // since we are in group-based iteration layout, it does not
        // matter which iteration we begin a step upon
        AdvanceStatus status = currentIteration.beginStep();
        if( status == AdvanceStatus::OVER )
        {
            *this = end();
            return *this;
        }
        currentIteration.setStepStatus( StepStatus::DuringStep );
        break;
    }
    default:
        break;
    }
    auto it = iterations.find( m_currentIteration );
    auto itEnd = iterations.end();
    if( it == itEnd )
    {
        *this = end();
        return *this;
    }
    ++it;
    if( it == itEnd )
    {
        *this = end();
        return *this;
    }
    m_currentIteration = it->first;
    switch( series.iterationEncoding() )
    {
        using IE = IterationEncoding;
    case IE::fileBased: {
        auto & iteration = series.iterations[ m_currentIteration ];
        AdvanceStatus status = iteration.beginStep();
        if( status == AdvanceStatus::OVER )
        {
            *this = end();
            return *this;
        }
        iteration.setStepStatus( StepStatus::DuringStep );
        break;
    }
    default:
        break;
    }
    return *this;
}

IndexedIteration SeriesIterator::operator*()
{
    return IndexedIteration(
        m_series.get().iterations[ m_currentIteration ], m_currentIteration );
}

bool SeriesIterator::operator==( SeriesIterator const & other ) const
{
    return this->m_currentIteration == other.m_currentIteration &&
        this->m_series.has_value() == other.m_series.has_value();
}

bool SeriesIterator::operator!=( SeriesIterator const & other ) const
{
    return !operator==( other );
}

SeriesIterator SeriesIterator::end()
{
    return {};
}

ReadIterations::ReadIterations( Series series )
    : m_series( std::move( series ) )
{
}

ReadIterations::iterator_t ReadIterations::begin()
{
    return iterator_t{ m_series };
}

ReadIterations::iterator_t ReadIterations::end()
{
    return SeriesIterator::end();
}
} // namespace openPMD
