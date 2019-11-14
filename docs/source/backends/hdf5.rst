.. _backends-hdf5:

HDF5 Backend
============

openPMD supports writing to and reading from HDF5 ``.h5`` files.
For this, the installed copy of openPMD must have been built with support for the HDF5 backend.
To build openPMD with support for HDF5, use the CMake option ``-DopenPMD_USE_HDF5=ON``.
For further information, check out the :ref:`installation guide <install>`,
:ref:`build dependencies <development-dependencies>` and the :ref:`build options <development-buildoptions>`.


I/O Method
----------

HDF5 internally either writes serially, via ``POSIX`` on Unix systems, or parallel to a single logical file via MPI-I/O.


Backend-Specific Controls
-------------------------

The following environment variables control HDF5 I/O behavior at runtime.

===================================== ========= ================================================================================
environment variable                  default   description
===================================== ========= ================================================================================
``OPENPMD_HDF5_INDEPENDENT``          ``OFF``   Sets the MPI-parallel transfer mode to collective or independent.
===================================== ========= ================================================================================

``OPENPMD_HDF5_INDEPENDENT``: by default, the HDF5's MPI-parallel write/read operations are `MPI-collective <https://www.mpi-forum.org/docs/mpi-2.2/mpi22-report/node87.htm#Node87>`_.
Since we call those underneath each ``storeChunk`` and ``loadChunk`` call, these methods become MPI-collective as well.
Please refer to the `HDF5 manual, function H5Pset_dxpl_mpio <https://support.hdfgroup.org/HDF5/doc/RM/H5P/H5Pset_dxpl_mpio.htm>`_ for more details.
In case you decide to use independent I/O with parallel HDF5, be advised that we did see performance penalties as well problems with some MPI-I/O implementations in the past.
For independent parallel I/O, potentially prefer using a modern version of the MPICH implementation (especially, use ROMIO instead of OpenMPI's ompio implementation).


Selected References
-------------------

* GitHub issue `#554 <https://github.com/openPMD/openPMD-api/pull/554>`_

* Axel Huebl, Rene Widera, Felix Schmitt, Alexander Matthes, Norbert Podhorszki, Jong Youl Choi, Scott Klasky, and Michael Bussmann.
  *On the Scalability of Data Reduction Techniques in Current and Upcoming HPC Systems from an Application Perspective,*
  ISC High Performance 2017: High Performance Computing, pp. 15-29, 2017.
  `arXiv:1706.00522 <https://arxiv.org/abs/1706.00522>`_, `DOI:10.1007/978-3-319-67630-2_2 <https://doi.org/10.1007/978-3-319-67630-2_2>`_