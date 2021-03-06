/* Copyright 2017-2021 Fabian Koller and Franz Poeschel
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
#pragma once

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#include "openPMD/IO/AbstractIOHandlerImplCommon.hpp"
#include "openPMD/IO/ADIOS/ADIOS2Auxiliary.hpp"
#include "openPMD/IO/ADIOS/ADIOS2FilePosition.hpp"
#include "openPMD/IO/ADIOS/ADIOS2PreloadAttributes.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/IO/InvalidatableFile.hpp"
#include "openPMD/auxiliary/JSON.hpp"
#include "openPMD/auxiliary/Option.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/config.hpp"

#if openPMD_HAVE_ADIOS2
#    include <adios2.h>
#endif
#if openPMD_HAVE_MPI
#   include <mpi.h>
#endif
#include <nlohmann/json.hpp>

#include <array>
#include <exception>
#include <future>
#include <iostream>
#include <memory> // shared_ptr
#include <set>
#include <string>
#include <unordered_map>
#include <utility> // pair
#include <vector>


namespace openPMD
{
#if openPMD_HAVE_ADIOS2

class ADIOS2IOHandler;

namespace detail
{
    template < typename, typename > struct DatasetHelper;
    struct DatasetReader;
    struct AttributeReader;
    struct AttributeWriter;
    struct OldAttributeReader;
    struct OldAttributeWriter;
    template < typename > struct AttributeTypes;
    struct DatasetOpener;
    struct VariableDefiner;
    template < typename > struct DatasetTypes;
    struct WriteDataset;
    struct BufferedActions;
    struct BufferedPut;
    struct BufferedGet;
    struct BufferedAttributeRead;
    struct BufferedAttributeWrite;
} // namespace detail


class ADIOS2IOHandlerImpl
: public AbstractIOHandlerImplCommon< ADIOS2FilePosition >
{
    template < typename, typename > friend struct detail::DatasetHelper;
    friend struct detail::DatasetReader;
    friend struct detail::AttributeReader;
    friend struct detail::AttributeWriter;
    friend struct detail::OldAttributeReader;
    friend struct detail::OldAttributeWriter;
    template < typename > friend struct detail::AttributeTypes;
    friend struct detail::DatasetOpener;
    friend struct detail::VariableDefiner;
    template < typename > friend struct detail::DatasetTypes;
    friend struct detail::WriteDataset;
    friend struct detail::BufferedActions;
    friend struct detail::BufferedAttributeRead;

    static constexpr bool ADIOS2_DEBUG_MODE = false;


public:

#if openPMD_HAVE_MPI

    ADIOS2IOHandlerImpl(
        AbstractIOHandler *,
        MPI_Comm,
        nlohmann::json config,
        std::string engineType );

#endif // openPMD_HAVE_MPI

    explicit ADIOS2IOHandlerImpl(
        AbstractIOHandler *,
        nlohmann::json config,
        std::string engineType );


    ~ADIOS2IOHandlerImpl() override;

    std::future< void > flush( ) override;

    void createFile( Writable *,
                     Parameter< Operation::CREATE_FILE > const & ) override;

    void createPath( Writable *,
                     Parameter< Operation::CREATE_PATH > const & ) override;

    void
    createDataset( Writable *,
                   Parameter< Operation::CREATE_DATASET > const & ) override;

    void
    extendDataset( Writable *,
                   Parameter< Operation::EXTEND_DATASET > const & ) override;

    void openFile( Writable *,
                   Parameter< Operation::OPEN_FILE > const & ) override;

    void closeFile( Writable *,
                    Parameter< Operation::CLOSE_FILE > const & ) override;

    void openPath( Writable *,
                   Parameter< Operation::OPEN_PATH > const & ) override;

    void closePath( Writable *,
                    Parameter< Operation::CLOSE_PATH > const & ) override;

    void openDataset( Writable *,
                      Parameter< Operation::OPEN_DATASET > & ) override;

    void deleteFile( Writable *,
                     Parameter< Operation::DELETE_FILE > const & ) override;

    void deletePath( Writable *,
                     Parameter< Operation::DELETE_PATH > const & ) override;

    void
    deleteDataset( Writable *,
                   Parameter< Operation::DELETE_DATASET > const & ) override;

    void deleteAttribute( Writable *,
                          Parameter< Operation::DELETE_ATT > const & ) override;

    void writeDataset( Writable *,
                       Parameter< Operation::WRITE_DATASET > const & ) override;

    void writeAttribute( Writable *,
                         Parameter< Operation::WRITE_ATT > const & ) override;

    void readDataset( Writable *,
                      Parameter< Operation::READ_DATASET > & ) override;

    void readAttribute( Writable *,
                        Parameter< Operation::READ_ATT > & ) override;

    void listPaths( Writable *, Parameter< Operation::LIST_PATHS > & ) override;

    void listDatasets( Writable *,
                       Parameter< Operation::LIST_DATASETS > & ) override;

    void
    listAttributes( Writable *,
                    Parameter< Operation::LIST_ATTS > & parameters ) override;

    void
    advance( Writable*, Parameter< Operation::ADVANCE > & ) override;

    void
    availableChunks( Writable*,
                     Parameter< Operation::AVAILABLE_CHUNKS > &) override;
    /**
     * @brief The ADIOS2 access type to chose for Engines opened
     * within this instance.
     */
    adios2::Mode adios2AccessMode( std::string const & fullPath );

private:
    adios2::ADIOS m_ADIOS;
    /**
     * The ADIOS2 engine type, to be passed to adios2::IO::SetEngine
     */
    std::string m_engineType;

    enum class AttributeLayout : char
    {
        ByAdiosAttributes,
        ByAdiosVariables
    };

    AttributeLayout m_attributeLayout = AttributeLayout::ByAdiosAttributes;

    struct ParameterizedOperator
    {
        adios2::Operator op;
        adios2::Params params;
    };

    std::vector< ParameterizedOperator > defaultOperators;

    auxiliary::TracingJSON m_config;
    static auxiliary::TracingJSON nullvalue;

    void
    init( nlohmann::json config );

    template< typename Key >
    auxiliary::TracingJSON
    config( Key && key, auxiliary::TracingJSON & cfg )
    {
        if( cfg.json().is_object() && cfg.json().contains( key ) )
        {
            return cfg[ key ];
        }
        else
        {
            return nullvalue;
        }
    }

    template< typename Key >
    auxiliary::TracingJSON
    config( Key && key )
    {
        return config< Key >( std::forward< Key >( key ), m_config );
    }

    /**
     *
     * @param config The top-level of the ADIOS2 configuration JSON object
     * with operators to be found under dataset.operators
     * @return first parameter: the operators, second parameters: whether
     * operators have been configured
     */
    auxiliary::Option< std::vector< ParameterizedOperator > >
    getOperators( auxiliary::TracingJSON config );

    // use m_config
    auxiliary::Option< std::vector< ParameterizedOperator > >
    getOperators();

    std::string
    fileSuffix() const;

    /*
     * We need to give names to IO objects. These names are irrelevant
     * within this application, since:
     * 1) The name of the file written to is decided by the opened Engine's
     *    name.
     * 2) The IOs are managed by the unordered_map m_fileData, so we do not
     *    need the ADIOS2 internal management.
     * Since within one m_ADIOS object, the same IO name cannot be used more
     * than once, we ensure different names by using the name counter.
     * This allows to overwrite a file later without error.
     */
    int nameCounter{0};

    /*
     * IO-heavy actions are deferred to a later point. This map stores for
     * each open file (identified by an InvalidatableFile object) an object
     * that manages IO-heavy actions, as well as its ADIOS2 objects, i.e.
     * IO and Engine object.
     * Not to be accessed directly, use getFileData().
     */
    std::unordered_map< InvalidatableFile,
                        std::unique_ptr< detail::BufferedActions >
    > m_fileData;

    std::map< std::string, adios2::Operator > m_operators;

    // Overrides from AbstractIOHandlerImplCommon.

    std::string
        filePositionToString( std::shared_ptr< ADIOS2FilePosition > ) override;

    std::shared_ptr< ADIOS2FilePosition >
    extendFilePosition( std::shared_ptr< ADIOS2FilePosition > const & pos,
                        std::string extend ) override;

    // Helper methods.

    auxiliary::Option< adios2::Operator >
    getCompressionOperator( std::string const & compression );

    /*
     * The name of the ADIOS2 variable associated with this Writable.
     * To be used for Writables that represent a dataset.
     */
    std::string nameOfVariable( Writable * writable );

    /**
     * @brief nameOfAttribute
     * @param writable The Writable at whose level the attribute lies.
     * @param attribute The openPMD name of the attribute.
     * @return The ADIOS2 name of the attribute, consisting of
     * the variable that the attribute is associated with
     * (possibly the empty string, representing no variable)
     * and the actual name.
     */
    std::string nameOfAttribute( Writable * writable, std::string attribute );

    /*
     * Figure out whether the Writable corresponds with a
     * group or a dataset.
     */
    ADIOS2FilePosition::GD groupOrDataset( Writable * );

    detail::BufferedActions & getFileData( InvalidatableFile file );

    void dropFileData( InvalidatableFile file );

    /*
     * Prepare a variable that already exists for an IO
     * operation, including:
     * (1) checking that its datatype matches T.
     * (2) the offset and extent match the variable's shape
     * (3) setting the offset and extent (ADIOS lingo: start
     *     and count)
     */
    template < typename T >
    adios2::Variable< T > verifyDataset( Offset const & offset,
                                         Extent const & extent, adios2::IO & IO,
                                         std::string const & var );
}; // ADIOS2IOHandlerImpl

/*
 * The following strings are used during parsing of the JSON configuration
 * string for the ADIOS2 backend.
 */
namespace ADIOS2Defaults
{
    using const_str = char const * const;
    constexpr const_str str_engine = "engine";
    constexpr const_str str_type = "type";
    constexpr const_str str_params = "parameters";
    constexpr const_str str_usesteps = "usesteps";
    constexpr const_str str_usesstepsAttribute = "__openPMD_internal/useSteps";
} // namespace ADIOS2Defaults

namespace detail
{
    // Helper structs for calls to the switchType function

    struct DatasetReader
    {
        openPMD::ADIOS2IOHandlerImpl * m_impl;


        explicit DatasetReader( openPMD::ADIOS2IOHandlerImpl * impl );


        template < typename T >
        void operator( )( BufferedGet & bp, adios2::IO & IO,
                          adios2::Engine & engine,
                          std::string const & fileName );

        std::string errorMsg = "ADIOS2: readDataset()";
    };

    struct OldAttributeReader
    {
        template< typename T >
        Datatype
        operator()(
            adios2::IO & IO,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        template< int n, typename... Params >
        Datatype
        operator()( Params &&... );
    };

    struct OldAttributeWriter
    {
        template< typename T >
        void
        operator()(
            ADIOS2IOHandlerImpl * impl,
            Writable * writable,
            const Parameter< Operation::WRITE_ATT > & parameters );


        template< int n, typename... Params >
        void
        operator()( Params &&... );
    };

    struct AttributeReader
    {
        template< typename T >
        Datatype
        operator()(
            adios2::IO & IO,
            detail::PreloadAdiosAttributes const & preloadedAttributes,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        template < int n, typename... Params >
        Datatype operator( )( Params &&... );
    };

    struct AttributeWriter
    {
        template < typename T >
        void
        operator()(
            detail::BufferedAttributeWrite & params,
            BufferedActions & fileData );


        template < int n, typename... Params > void operator( )( Params &&... );
    };

    struct DatasetOpener
    {
        ADIOS2IOHandlerImpl * m_impl;

        explicit DatasetOpener( ADIOS2IOHandlerImpl * impl );


        template < typename T >
        void operator( )( InvalidatableFile, const std::string & varName,
                          Parameter< Operation::OPEN_DATASET > & parameters );


        std::string errorMsg = "ADIOS2: openDataset()";
    };

    struct WriteDataset
    {
        ADIOS2IOHandlerImpl * m_handlerImpl;


        WriteDataset( ADIOS2IOHandlerImpl * handlerImpl );


        template < typename T >
        void operator( )( BufferedPut & bp, adios2::IO & IO,
                          adios2::Engine & engine );

        template < int n, typename... Params > void operator( )( Params &&... );
    };

    struct VariableDefiner
    {
        /**
         * @brief Define a Variable of type T within the passed IO.
         *
         * @param IO The adios2::IO object within which to define the
         *           variable. The variable can later be retrieved from
         *           the IO using the passed name.
         * @param name As in adios2::IO::DefineVariable
         * @param compressions ADIOS2 operators, including an arbitrary
         *                     number of parameters, to be added to the
         *                     variable upon definition.
         * @param shape As in adios2::IO::DefineVariable
         * @param start As in adios2::IO::DefineVariable
         * @param count As in adios2::IO::DefineVariable
         * @param constantDims As in adios2::IO::DefineVariable
         */
        template < typename T >
        void operator( )(
            adios2::IO & IO,
            std::string const & name,
            std::vector< ADIOS2IOHandlerImpl::ParameterizedOperator > const &
                compressions,
            adios2::Dims const & shape = adios2::Dims(),
            adios2::Dims const & start = adios2::Dims(),
            adios2::Dims const & count = adios2::Dims(),
            bool const constantDims = false );

        std::string errorMsg = "ADIOS2: defineVariable()";
    };

    struct RetrieveBlocksInfo
    {
        template < typename T >
        void operator( )(
            Parameter< Operation::AVAILABLE_CHUNKS > & params,
            adios2::IO & IO,
            adios2::Engine & engine,
            std::string const & varName );

        template < int n, typename... Params >
        void operator( )( Params &&... );
    };

    // Helper structs to help distinguish valid attribute/variable
    // datatypes from invalid ones

    /*
     * This struct's purpose is to have specialisations
     * for vector and array types, as well as the boolean
     * type (which is not natively supported by ADIOS).
     */
    template< typename T >
    struct AttributeTypes
    {
        static void
        oldCreateAttribute(
            adios2::IO & IO,
            std::string name,
            T value );

        static void
        oldReadAttribute(
            adios2::IO & IO,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        static void
        createAttribute(
            adios2::IO & IO,
            adios2::Engine & engine,
            detail::BufferedAttributeWrite & params,
            T value );

        static void
        readAttribute(
            detail::PreloadAdiosAttributes const &,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        /**
         * @brief Is the attribute given by parameters name and val already
         *        defined exactly in that way within the given IO?
         */
        static bool
        attributeUnchanged( adios2::IO & IO, std::string name, T val )
        {
            auto attr = IO.InquireAttribute< T >( name );
            if( !attr )
            {
                return false;
            }
            std::vector< T > data = attr.Data();
            if( data.size() != 1 )
            {
                return false;
            }
            return data[ 0 ] == val;
        }
    };

    template< > struct AttributeTypes< std::complex< long double > >
    {
        static void
        oldCreateAttribute(
            adios2::IO &,
            std::string,
            std::complex< long double > )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex attribute types" );
        }

        static void
        oldReadAttribute(
            adios2::IO &,
            std::string,
            std::shared_ptr< Attribute::resource > )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex attribute types" );
        }

        static void
        createAttribute(
            adios2::IO &,
            adios2::Engine &,
            detail::BufferedAttributeWrite &,
            std::complex< long double > )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex attribute types" );
        }

        static void
        readAttribute(
            detail::PreloadAdiosAttributes const &,
            std::string,
            std::shared_ptr< Attribute::resource > )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex attribute types" );
        }

        static bool
        attributeUnchanged(
            adios2::IO &, std::string, std::complex< long double > )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex attribute types" );
        }
    };

    template< > struct AttributeTypes< std::vector< std::complex< long double > > >
    {
        static void
        oldCreateAttribute(
            adios2::IO &,
            std::string,
            const std::vector< std::complex< long double > > & )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex vector attribute types" );
        }

        static void
        oldReadAttribute(
            adios2::IO &,
            std::string,
            std::shared_ptr< Attribute::resource > )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex vector attribute types" );
        }

        static void
        createAttribute(
            adios2::IO &,
            adios2::Engine &,
            detail::BufferedAttributeWrite &,
            const std::vector< std::complex< long double > > & )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex vector attribute types" );
        }

        static void
        readAttribute(
            detail::PreloadAdiosAttributes const &,
            std::string,
            std::shared_ptr< Attribute::resource > )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex vector attribute types" );
        }

        static bool
        attributeUnchanged(
            adios2::IO &,
            std::string,
            std::vector< std::complex< long double > > )
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex vector attribute types" );
        }
    };

    template < typename T > struct AttributeTypes< std::vector< T > >
    {
        static void
        oldCreateAttribute(
            adios2::IO & IO,
            std::string name,
            const std::vector< T > & value );

        static void
        oldReadAttribute(
            adios2::IO & IO,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        static void
        createAttribute(
            adios2::IO & IO,
            adios2::Engine & engine,
            detail::BufferedAttributeWrite & params,
            const std::vector< T > & value );

        static void
        readAttribute(
            detail::PreloadAdiosAttributes const &,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        static bool
        attributeUnchanged(
            adios2::IO & IO,
            std::string name,
            std::vector< T > val )
        {
            auto attr = IO.InquireAttribute< T >( name );
            if( !attr )
            {
                return false;
            }
            std::vector< T > data = attr.Data();
            if( data.size() != val.size() )
            {
                return false;
            }
            for( size_t i = 0; i < val.size(); ++i )
            {
                if( data[ i ] != val[ i ] )
                {
                    return false;
                }
            }
            return true;
        }
    };

    template<>
    struct AttributeTypes< std::vector< std::string > >
    {
        static void
        oldCreateAttribute(
            adios2::IO & IO,
            std::string name,
            const std::vector< std::string > & value );

        static void
        oldReadAttribute(
            adios2::IO & IO,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        static void
        createAttribute(
            adios2::IO & IO,
            adios2::Engine & engine,
            detail::BufferedAttributeWrite & params,
            const std::vector< std::string > & vec );

        static void
        readAttribute(
            detail::PreloadAdiosAttributes const &,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        static bool
        attributeUnchanged(
            adios2::IO & IO,
            std::string name,
            std::vector< std::string > val )
        {
            auto attr = IO.InquireAttribute< std::string >( name );
            if( !attr )
            {
                return false;
            }
            std::vector< std::string > data = attr.Data();
            if( data.size() != val.size() )
            {
                return false;
            }
            for( size_t i = 0; i < val.size(); ++i )
            {
                if( data[ i ] != val[ i ] )
                {
                    return false;
                }
            }
            return true;
        }
    };

    template < typename T, size_t n >
    struct AttributeTypes< std::array< T, n > >
    {
        static void
        oldCreateAttribute(
            adios2::IO & IO,
            std::string name,
            const std::array< T, n > & value );

        static void
        oldReadAttribute(
            adios2::IO & IO,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        static void
        createAttribute(
            adios2::IO & IO,
            adios2::Engine & engine,
            detail::BufferedAttributeWrite & params,
            const std::array< T, n > & value );

        static void
        readAttribute(
            detail::PreloadAdiosAttributes const &,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        static bool
        attributeUnchanged(
            adios2::IO & IO,
            std::string name,
            std::array< T, n > val )
        {
            auto attr = IO.InquireAttribute< T >( name );
            if( !attr )
            {
                return false;
            }
            std::vector< T > data = attr.Data();
            if( data.size() != n )
            {
                return false;
            }
            for( size_t i = 0; i < n; ++i )
            {
                if( data[ i ] != val[ i ] )
                {
                    return false;
                }
            }
            return true;
        }
    };

    template <> struct AttributeTypes< bool >
    {
        using rep = detail::bool_representation;

        static void
        oldCreateAttribute( adios2::IO & IO, std::string name, bool value );

        static void
        oldReadAttribute(
            adios2::IO & IO,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );

        static void
        createAttribute(
            adios2::IO & IO,
            adios2::Engine & engine,
            detail::BufferedAttributeWrite & params,
            bool value );

        static void
        readAttribute(
            detail::PreloadAdiosAttributes const &,
            std::string name,
            std::shared_ptr< Attribute::resource > resource );


        static constexpr rep toRep( bool b )
        {
            return b ? 1U : 0U;
        }


        static constexpr bool fromRep( rep r )
        {
            return r != 0;
        }

        static bool
        attributeUnchanged( adios2::IO & IO, std::string name, bool val )
        {
            auto attr = IO.InquireAttribute< rep >( name );
            if( !attr )
            {
                return false;
            }
            std::vector< rep > data = attr.Data();
            if( data.size() != 1 )
            {
                return false;
            }
            return data[ 0 ] == toRep( val );
        }
    };

    // Other datatypes used in the ADIOS2IOHandler implementation


    struct BufferedActions;

    /*
     * IO-heavy action to be executed upon flushing.
     */
    struct BufferedAction
    {
        virtual ~BufferedAction( ) = default;

        virtual void run( BufferedActions & ) = 0;
    };

    struct BufferedGet : BufferedAction
    {
        std::string name;
        Parameter< Operation::READ_DATASET > param;

        void run( BufferedActions & ) override;
    };

    struct BufferedPut : BufferedAction
    {
        std::string name;
        Parameter< Operation::WRITE_DATASET > param;

        void run( BufferedActions & ) override;
    };

    struct OldBufferedAttributeRead : BufferedAction
    {
        Parameter< Operation::READ_ATT > param;
        std::string name;

        void run( BufferedActions & ) override;
    };

    struct BufferedAttributeRead
    {
        Parameter< Operation::READ_ATT > param;
        std::string name;

        void
        run( BufferedActions & );
    };

    struct BufferedAttributeWrite
    {
        std::string name;
        Datatype dtype;
        Attribute::resource resource;
        std::vector< char > bufferForVecString;

        void
        run( BufferedActions & );
    };

    /*
     * Manages per-file information about
     * (1) the file's IO and Engine objects
     * (2) the file's deferred IO-heavy actions
     */
    struct BufferedActions
    {
        BufferedActions( BufferedActions const & ) = delete;

        /**
         * The full path to the file created on disk, including the
         * containing directory and the file extension, as determined
         * by ADIOS2IOHandlerImpl::fileSuffix().
         * (Meaning, in case of the SST engine, no file suffix since the
         *  SST engine automatically adds its suffix unconditionally)
         */
        std::string m_file;
        /**
         * ADIOS requires giving names to instances of adios2::IO.
         * We make them different from the actual file name, because of the
         * possible following workflow:
         *
         * 1. create file foo.bp
         *    -> would create IO object named foo.bp
         * 2. delete that file
         *    (let's ignore that we don't support deletion yet and call it
         *     preplanning)
         * 3. create file foo.bp a second time
         *    -> would create another IO object named foo.bp
         *    -> craash
         *
         * So, we just give out names based on a counter for IO objects.
         * Hence, next to the actual file name, also store the name for the
         * IO.
         */
        std::string const m_IOName;
        adios2::ADIOS & m_ADIOS;
        adios2::IO m_IO;
        std::vector< std::unique_ptr< BufferedAction > > m_buffer;
        std::map< std::string, BufferedAttributeWrite > m_attributeWrites;
        std::vector< BufferedAttributeRead > m_attributeReads;
        adios2::Mode m_mode;
        detail::WriteDataset const m_writeDataset;
        detail::DatasetReader const m_readDataset;
        detail::AttributeReader const m_attributeReader;
        PreloadAdiosAttributes preloadAttributes;
        using AttributeLayout = ADIOS2IOHandlerImpl::AttributeLayout;
        AttributeLayout m_attributeLayout = AttributeLayout::ByAdiosAttributes;

        /*
         * We call an attribute committed if the step during which it was
         * written has been closed.
         * A committed attribute cannot be modified.
         */
        std::set< std::string > uncommittedAttributes;

        /*
         * The openPMD API will generally create new attributes for each
         * iteration. This results in a growing number of attributes over time.
         * In streaming-based modes, these will be completely sent anew in each
         * iteration. If the following boolean is true, old attributes will be
         * removed upon CLOSE_GROUP.
         * Should not be set to true in persistent backends.
         * Will be automatically set by BufferedActions::configure_IO depending
         * on chosen ADIOS2 engine and can not be explicitly overridden by user.
         */
        bool optimizeAttributesStreaming = false;

        using AttributeMap_t = std::map< std::string, adios2::Params >;

        BufferedActions( ADIOS2IOHandlerImpl & impl, InvalidatableFile file );

        ~BufferedActions( );

        /**
         * Implementation of destructor, will only run once.
         *
         */
        void
        finalize();

        adios2::Engine & getEngine( );
        adios2::Engine & requireActiveStep( );

        template < typename BA > void enqueue( BA && ba );

        template < typename BA > void enqueue( BA && ba, decltype( m_buffer ) & );

        /**
         * Flush deferred IO actions.
         *
         * @param performPutsGets A functor that takes as parameters (1) *this
         *     and (2) the ADIOS2 engine.
         *     Its task is to ensure that ADIOS2 performs Put/Get operations.
         *     Several options for this:
         *     * adios2::Engine::EndStep
         *     * adios2::Engine::Perform(Puts|Gets)
         *     * adios2::Engine::Close
         * @param writeAttributes If using the new attribute layout, perform
         *     deferred attribute writes now.
         * @param flushUnconditionally Whether to run the functor even if no
         *     deferred IO tasks had been queued.
         */
        template< typename F >
        void
        flush(
            F && performPutsGets,
            bool writeAttributes,
            bool flushUnconditionally );

        /**
         * Overload of flush() that uses adios2::Engine::Perform(Puts|Gets)
         * and does not flush unconditionally.
         *
         */
        void
        flush( bool writeAttributes = false );

        /**
         * @brief Begin or end an ADIOS step.
         *
         * @param mode Whether to begin or end a step.
         * @return AdvanceStatus
         */
        AdvanceStatus
        advance( AdvanceMode mode );

        /*
         * Delete all buffered actions without running them.
         */
        void drop( );

        AttributeMap_t const &
        availableAttributes();

        std::vector< std::string >
        availableAttributesPrefixed( std::string const & prefix );

        /*
         * See description below.
         */
        void
        invalidateAttributesMap();

        AttributeMap_t const &
        availableVariables();

        std::vector< std::string >
        availableVariablesPrefixed( std::string const & prefix );

        /*
         * See description below.
         */
        void
        invalidateVariablesMap();

    private:
        auxiliary::Option< adios2::Engine > m_engine; //! ADIOS engine
        /**
         * The ADIOS2 engine type, to be passed to adios2::IO::SetEngine
         */
        std::string m_engineType;
        /*
         * streamStatus is NoStream for file-based ADIOS engines.
         * This is relevant for the method BufferedActions::requireActiveStep,
         * where a step is only opened if the status is OutsideOfStep, but not
         * if NoStream. The rationale behind this is that parsing a Series
         * works differently for file-based and for stream-based engines:
         * * stream-based: Iterations are parsed as they arrive. For parsing an
         *   iteration, the iteration must be awaited.
         *   BufferedActions::requireActiveStep takes care of this.
         * * file-based: The Series is parsed up front. If no step has been
         *   opened yet, ADIOS2 gives access to all variables and attributes
         *   from all steps. Upon opening a step, only the variables from that
         *   step are shown which hinders parsing. So, until a step is
         *   explicitly opened via ADIOS2IOHandlerImpl::advance, do not open
         *   one.
         *   This is to enable use of ADIOS files without the Streaming API
         *   (i.e. all iterations should be visible to the user upon opening
         *   the Series.)
         *   @todo Add a workflow without up-front parsing of all iterations
         *         for file-based engines.
         *         (This would merely be an optimization since the streaming
         *         API still works with files as intended.)
         *
         */
        enum class StreamStatus
        {
            /**
             * A step is currently active.
             */
            DuringStep,
            /**
             * A stream is active, but no step.
             */
            OutsideOfStep,
            /**
             * Stream has ended.
             */
            StreamOver,
            /**
             * File is not written is streaming fashion.
             * Begin/EndStep will be replaced by simple flushes.
             * Used for:
             * 1) Writing BP4 files without steps despite using the Streaming
             *    API. This is due to the fact that ADIOS2.6.0 requires using
             *    steps to read BP4 files written with steps, so using steps
             *    is opt-in for now.
             *    Notice that while the openPMD API requires ADIOS >= 2.7.0,
             *    the resulting files need to be readable from ADIOS 2.6.0 as
             *    well. This workaround is hence staying until switching to
             *    a new ADIOS schema.
             * 2) Reading with the Streaming API any file that has been written
             *    without steps. This is not a workaround since not using steps,
             *    while inefficient in ADIOS2, is something that we support.
             */
            NoStream,
            /**
             * Rationale behind this state:
             * When user code opens a Series, series.iterations should contain
             * all available iterations.
             * If accessing a file without opening a step, ADIOS2 will grant
             * access to variables and attributes from all steps, allowing us
             * to parse the complete dump.
             * This state indicates that no step should be opened for parsing
             * purposes (which is necessary in streaming engines, hence they
             * are initialized with the OutsideOfStep state).
             * A step should only be opened if an explicit ADVANCE task arrives
             * at the backend.
             * 
             * @todo If the streaming API is used on files, parsing the whole
             *       Series up front is unnecessary work.
             *       Our frontend does not yet allow to distinguish whether
             *       parsing the whole series will be necessary since parsing
             *       happens upon construction time of Series,
             *       but the classical and the streaming API are both activated
             *       afterwards from the created Series object.
             *       Hence, improving this requires refactoring in our
             *       user-facing API. Ideas:
             *       (1) Delayed lazy parsing of iterations upon accessing
             *           (would bring other benefits also).
             *       (2) Introduce a restricted class StreamingSeries.
             */
            Parsing,
            /**
             * The stream status of a file-based engine will be decided upon
             * opening the engine if in read mode. Up until then, this right
             * here is the status.
             */
            Undecided
        };
        StreamStatus streamStatus = StreamStatus::OutsideOfStep;
        adios2::StepStatus m_lastStepStatus = adios2::StepStatus::OK;

        /**
         * See documentation for StreamStatus::Parsing.
         * Will be set true under the circumstance described there in order to
         * indicate that the first step should only be opened after parsing.
         */
        bool delayOpeningTheFirstStep = false;

        /*
         * ADIOS2 does not give direct access to its internal attribute and
         * variable maps, but will instead give access to copies of them.
         * In order to avoid unnecessary copies, we buffer the returned map.
         * The downside of this is that we need to pay attention to invalidate
         * the map whenever an attribute/variable is altered. In that case, we
         * fetch the map anew.
         * If empty, the buffered map has been invalidated and needs to be
         * queried from ADIOS2 again. If full, the buffered map is equivalent to
         * the map that would be returned by a call to
         * IO::Available(Attributes|Variables).
         */
        auxiliary::Option< AttributeMap_t > m_availableAttributes;
        auxiliary::Option< AttributeMap_t > m_availableVariables;

        /*
         * finalize() will set this true to avoid running twice.
         */
        bool finalized = false;

        void
        configure_IO( ADIOS2IOHandlerImpl & impl );
    };


} // namespace detail
#endif // openPMD_HAVE_ADIOS2


class ADIOS2IOHandler : public AbstractIOHandler
{
#if openPMD_HAVE_ADIOS2

friend class ADIOS2IOHandlerImpl;

private:
    ADIOS2IOHandlerImpl m_impl;

public:
    ~ADIOS2IOHandler( ) override
    {
        // we must not throw in a destructor
        try
        {
            this->flush( );
        }
        catch( std::exception const & ex )
        {
            std::cerr << "[~ADIOS2IOHandler] An error occurred: " << ex.what() << std::endl;
        }
        catch( ... )
        {
            std::cerr << "[~ADIOS2IOHandler] An error occurred." << std::endl;
        }
    }

#else
public:
#endif

#if openPMD_HAVE_MPI

    ADIOS2IOHandler(
        std::string path,
        Access,
        MPI_Comm,
        nlohmann::json options,
        std::string engineType );

#endif

    ADIOS2IOHandler(
        std::string path,
        Access,
        nlohmann::json options,
        std::string engineType );

    std::string backendName() const override { return "ADIOS2"; }

    std::future< void > flush( ) override;
}; // ADIOS2IOHandler
} // namespace openPMD
