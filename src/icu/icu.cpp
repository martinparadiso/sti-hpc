#include "../icu.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/json/object.hpp>
#include <fstream>
#include <repast_hpc/Properties.h>
#include <repast_hpc/Random.h>
#include <repast_hpc/RepastProcess.h>
#include <sstream>

#include "../space_wrapper.hpp"
#include "../hospital_plan.hpp"
#include "../json_serialization.hpp"
#include "../agent_factory.hpp"
#include "real_icu.hpp"
#include "proxy_icu.hpp"

////////////////////////////////////////////////////////////////////////////
// CONSTRUCTION
////////////////////////////////////////////////////////////////////////////

/// @brief Construct an ICU, let the construct figure out which one
/// @param communicator The MPI communicator
/// @param mpi_tag The MPI tag for the communication
/// @param hospital_props The hospital properties stored in a JSON object
/// @param hospital_plan The hospital plan.
/// @param space The space_wrapper
/// @param clock The simulation clock
sti::icu::icu(boost::mpi::communicator*  communicator,
              const repast::Properties&  execution_props,
              const boost::json::object& hospital_props,
              const hospital_plan&       hospital_plan,
              space_wrapper*             space,
              clock*                     clock)
    : _real_icu { [&]() -> decltype(_real_icu) {
        // If the ICU is physically in this process create a real ICU. Ownership
        // is set in the next construct.
        
        if (space->local_dimensions().contains(hospital_plan.icu().location)) {
            return new real_icu { communicator, mpi_tag, hospital_props, hospital_plan, clock };
        }

        return nullptr;
    }() }
    , _icu_admission { [&]() -> decltype(_icu_admission)::pointer {
        // If the real_icu is initialized, use the same pointer to initialize
        // the ownership. Otherwise this is a remote/proxy process and a
        // proxy_icu has to be created.
        // Only the root knows the where is the real ICU, so the real_rank has
        // to be sent to all the other processes.
        if (_real_icu != nullptr) {
            for (auto p = 0; p < communicator->size(); ++p) {
                if (p != communicator->rank()) {
                    communicator->send(p, 68542, communicator->rank());
                }
            }
            return _real_icu;
        }
        
        // If the real_icu is not here, wait for the message containing the
        // real icu rank and create a proxy
        auto real_rank = 0;
        communicator->recv(boost::mpi::any_source, 68542, real_rank);
        return new proxy_icu { communicator, mpi_tag, real_rank };
    }() }
{

}

sti::icu::~icu() = default;

////////////////////////////////////////////////////////////////////////////
// ADMISSION MANAGEMENT
////////////////////////////////////////////////////////////////////////////

/// @brief Get the admsission system, to add and remove agents
/// @return A reference to the admission
sti::icu_admission& sti::icu::admission()
{
    return *_icu_admission;
}

/// @brief Get the admsission system, to add and remove agents
/// @return A const reference to the admission
const sti::icu_admission& sti::icu::admission() const
{
    return *_icu_admission;
}

////////////////////////////////////////////////////////////////////////////
// REAL ICU ACCESS
////////////////////////////////////////////////////////////////////////////

/// @brief Get a reference to the real ICU, if it resides in this process
/// @return An optional containing a reference to the real ICU
std::optional<std::reference_wrapper<sti::real_icu>> sti::icu::get_real_icu()
{
    if (_real_icu != nullptr) return { *_real_icu };
    return {};
}

/// @brief Get a reference to the real ICU, if it resides in this process
/// @return An optional containing a const reference to the real ICU
std::optional<std::reference_wrapper<const sti::real_icu>> sti::icu::get_real_icu() const
{
    if (_real_icu != nullptr) return { *_real_icu };
    return {};
}

////////////////////////////////////////////////////////////////////////////
// STATISTICS COLLECTION
////////////////////////////////////////////////////////////////////////////

/// @brief Save the ICU stats into a file
/// @param filepath The path to the folder where
void sti::icu::save(const std::string& folderpath) const
{
    // If the real_icu is here, save the stats
    if (_real_icu != nullptr) {
        _real_icu->save(folderpath);
    }
}
