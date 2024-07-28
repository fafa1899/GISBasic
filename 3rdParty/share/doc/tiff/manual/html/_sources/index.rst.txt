####################################
LibTIFF - TIFF Library and Utilities
####################################

.. table:: References
    :widths: auto

    =====================  =====================================
    Name                   Reference
    =====================  =====================================
    Home Page #1           http://www.libtiff.org
    Latest Stable Release  :doc:`v4.6.0t </releases/v4.6.0t>`
    Master Download Site   `www.libtiff.org/downloads <http://www.libtiff.org/downloads/>`_
    Source repository      https://gitlab.com/faxguy/libtiff-tools
    .                      .
    **Upstream** **        **Reference**
    Home Page #1           http://www.simplesystems.org/libtiff/
    Home Page #2           https://libtiff.gitlab.io/libtiff/
    Latest Stable Release  v4.6.0 (:tag:`v4.6.0`)
    Master Download Site   `download.osgeo.org <https://download.osgeo.org/libtiff/>`_
    Mailing List           `tiff@lists.osgeo.org <tiff@lists.osgeo.org>`_
    List subscription      http://lists.osgeo.org/mailman/listinfo/tiff/
    List archive           http://www.awaresystems.be/imaging/tiff/tml.html
    Source repository      https://gitlab.com/libtiff/libtiff
    =====================  =====================================

The LibTIFF software provides support for the *Tag Image File Format* (TIFF),
a widely used format for storing image data.  The latest version of
the TIFF specification is :doc:`specification/index`.

Included in this software distribution are:

* a library, :file:`libtiff`, for reading and writing TIFF images
* a small collection of tools for performing simple manipulations of TIFF images
* documentation for the library and tools.

The :file:`libtiff` library, along with associated tool programs, should handle most of
your needs for reading and writing TIFF images.

LibTIFF is portable software, supported on various operating systems including
UNIX (Linux, BSD, MacOS X) and Windows.

** Upstream libtiff development currently excludes most of the tools and may have
   fewer features available in the tools not excluded.  Users of the tools should
   ensure that their source is derived from the repository which includes tools.


The following sections are included in this documentation:

.. toctree::
    :maxdepth: 2
    :titlesonly:

    specification/index
    images
    build
    terms
    libtiff
    multi_page
    functions
    internals
    addingtags
    tools
    contrib
    project/index
    releases/index
