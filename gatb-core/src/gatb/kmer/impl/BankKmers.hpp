/*****************************************************************************
 *   GATB : Genome Assembly Tool Box
 *   Copyright (C) 2014  INRIA
 *   Authors: R.Chikhi, G.Rizk, E.Drezen
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

/** \file BankKmers.hpp
 *  \date 01/03/2013
 *  \author edrezen
 *  \brief Kmer iterator on sequences of a bank
 */

#ifndef _GATB_CORE_KMER_IMPL_BANK_KMERS_HPP_
#define _GATB_CORE_KMER_IMPL_BANK_KMERS_HPP_

/********************************************************************************/

#include <gatb/kmer/impl/Model.hpp>
#include <gatb/bank/impl/AbstractBank.hpp>
#include <cmath>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace kmer      {
namespace impl      {
/********************************************************************************/

/** \brief Bank whose sequences are all the possible kmers of a kmer model.
 */
class BankKmers : public bank::impl::AbstractBank
{
public:

    /** */
    BankKmers (size_t kmerSize) : _model(kmerSize)
    {
        _totalNumber = ((u_int64_t)1) << (2*_model.getKmerSize());
    }

    /** Get an unique identifier for the bank (could be an URI for instance).
     * \return the identifier */
    std::string getId ()  {  std::stringstream ss; ss << "Kmers" << _model.getKmerSize();  return ss.str();  }

    /** */
    int64_t getNbItems () { return _totalNumber; }

    /** */
    void estimate (u_int64_t& number, u_int64_t& totalSize, u_int64_t& maxSize)
    {
        number    = _totalNumber;
        totalSize = _totalNumber * _model.getKmerSize();
        maxSize   = _model.getKmerSize();
    }

    /** \copydoc tools::collections::Bag */
    void insert (const bank::Sequence& item)  { throw system::Exception ("Can't insert sequence to BankKmers"); }

    /** \copydoc tools::collections::Bag */
    void flush ()  { throw system::Exception ("Can't flush BankKmers"); }

    /** */
    u_int64_t getSize () { return _totalNumber * _model.getKmerSize(); }

    /** */
    class Iterator : public tools::dp::Iterator<bank::Sequence>
    {
    public:
        /** Constructor.
         * \param[in] ref : the associated iterable instance.
         */
        Iterator (BankKmers& ref) : _ref(ref), _isDone(true), _kmer(0), _kmerMax(_ref._totalNumber) {}

        /** Destructor */
        virtual ~Iterator () {}

        /** \copydoc tools::dp::Iterator::first */
        void first()
        {
            _kmer = 0;
            _isDone = (_kmer >= _kmerMax);
            if (!_isDone)  { updateSequence (); }
        }

        /** \copydoc tools::dp::Iterator::next */
        void next()
        {
            _kmer += 1;
            _isDone = (_kmer >= _kmerMax);
            if (!_isDone)  { updateSequence (); }
        }

        /** \copydoc tools::dp::Iterator::isDone */
        bool isDone ()  { return _isDone; }

        /** \copydoc tools::dp::Iterator::item */
        bank::Sequence& item ()     { return *_item; }

    private:

        /** Reference to the underlying bank. */
        BankKmers&    _ref;

        /** Tells whether the iteration is finished or not. */
        bool _isDone;

        /** */
        u_int64_t _kmer;
        u_int64_t _kmerMax;

        /** */
        void updateSequence ()
        {
            /** We get the string representation of the current kmer. */
            _kmerString = _ref._model.toString(_kmer);

            /** We build the comment of the sequence. */
            _ss.str ("");  _ss << "seq_" << _kmer;
            _item->setComment (_ss.str().c_str());

            /** We set the data for the sequence.
             * NOTE : we use 'set' and not 'setRef' since we want a true copy.
             * => mandatory in case this iterator is used through a parallel dispatcher. */
            _item->getData().set ((char*)_kmerString.data(), _kmerString.size());
        }

        /** */
        std::string       _kmerString;
        std::stringstream _ss;
    };

    /** \copydoc tools::collections::Iterable::iterator */
    tools::dp::Iterator<bank::Sequence>* iterator ()  { return new Iterator (*this); }

private:
    Kmer<>::ModelCanonical _model;
    u_int64_t              _totalNumber;

    friend class Iterator;
};

/********************************************************************************/

/** Statistics about the bank. */
struct BankStats
{
    BankStats ()
    : sequencesNb(0), sequencesMinLength(~0), sequencesMaxLength(0), sequencesTotalLength(0), sequencesTotalLengthSquare(0),
      kmersNbValid(0), kmersNbInvalid(0) {}

    void update (bank::Sequence& sequence)
    {
        sequencesNb++;
        sequencesTotalLength       += sequence.getDataSize();
        sequencesTotalLengthSquare += sequence.getDataSize() * sequence.getDataSize();
        if (sequencesMinLength > sequence.getDataSize())  {  sequencesMinLength = sequence.getDataSize(); }
        if (sequencesMaxLength < sequence.getDataSize())  {  sequencesMaxLength = sequence.getDataSize(); }
    }

    BankStats& operator+= (const BankStats& other)
    {
        sequencesNb                += other.sequencesNb;
        sequencesTotalLength       += other.sequencesTotalLength;
        sequencesTotalLengthSquare += other.sequencesTotalLengthSquare;
        kmersNbValid               += other.kmersNbValid;
        kmersNbInvalid             += other.kmersNbInvalid;
        sequencesMinLength          = std::min (sequencesMinLength, other.sequencesMinLength);
        sequencesMaxLength          = std::max (sequencesMaxLength, other.sequencesMaxLength);
        return *this;
    }

    /** */
    double getSeqMean ()  {  return (sequencesNb > 0 ?  (double)sequencesTotalLength / (double)sequencesNb : 0.0);  }

    /** */
    double getSeqDeviation ()
    {
        double result = 0.0;
        if (sequencesNb > 0)  {  result = sqrt ((double)sequencesTotalLengthSquare / (double)sequencesNb - pow(getSeqMean(),2));  }
        return result;
    }

    u_int64_t sequencesNb;
    u_int64_t sequencesMinLength;
    u_int64_t sequencesMaxLength;
    u_int64_t sequencesTotalLength;
    u_int64_t sequencesTotalLengthSquare;
    u_int64_t kmersNbValid;
    u_int64_t kmersNbInvalid;
};

/********************************************************************************/
} } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_KMER_IMPL_BANK_KMERS_HPP_ */
