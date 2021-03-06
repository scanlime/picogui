""" PGBuild.Repository

Contains the Repository class that links a URI with a working directory,
with methods to determine if the working copy is up to date and update it.
"""
# 
# PicoGUI Build System
# Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
# 
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#  
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#  
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
# 
_svn_id = "$Id$"

def open(ctx, url):
    """Repository factory- given a URL, this guesses what type of repository
       it points to and instantiates the proper Repository object for it.
       """
    import urlparse
    (scheme, server, path, parameters, query, fragment) = urlparse.urlparse(url)
    repoFactory = None

    # Anything ending in .tar, .tar.gz, or .tar.bz2 is a tar file
    if path[-7:] == ".tar.gz" or path[-8:] == ".tar.bz2" or path[-4:] == ".tar":
        import PGBuild.Repository.Tar
        repoClass = PGBuild.Repository.Tar.Repository

    # Assume any remaining URLs with a http, https, or svn scheme are subversion repositories
    if scheme == "http" or scheme == "https" or scheme == "svn":
        import PGBuild.Repository.Subversion
        repoFactory = PGBuild.Repository.Subversion.Repository

    if not repoFactory:
        import PGBuild.Errors
        raise PGBuild.Errors.ConfigError("Unable to determine repository type for the URL '%s'" % url)

    return repoFactory(ctx, url)


class RepositoryBase(object):
    """Base class for all Repository implementations"""
    def __init__(self, ctx, url):
        """Constructor, binds this repository to one URL. This shouldn't actually
           perform any network I/O, so it won't yet know if the URL is valid.
           """
        pass
    
    def download(self, ctx, destination):
        """Force the repository to be downloaded in its entirety to the given destination,
           outputting progress information to the given baton's progress object.
           """
        pass
    
    def isUpdateAvailable(self, ctx, destination):
        """Check whether an update is available"""
        pass

    def update(self, ctx, destination):
        """Update the local copy at the given destination if there's an update available,
           creating the destination directory if necessary. Progress is output to the given
           progress object. Returns nonzero if there was an update.
           """
        pass

    def isLocalCopyValid(self, ctx, destination):
        """Check whether the local copy is valid. A local copy is still valid if it's
           older than the repository, so this should not perform any network access.
           """
        # Unless the repository implementation has something better to do, we'll just
        # check whether the directory exists.
        import os
        return os.path.isdir(destination)

### The End ###

