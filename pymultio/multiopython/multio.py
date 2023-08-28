import os

from .lib import ffi, lib
from .metadata import Metadata

class _Config:
    """This is the main container class for Multio Configs"""

    def __init__(self, config_path, allow_world, parent_comm, client_comm, server_comm):
        self.__config_path = config_path

        config = ffi.new("multio_configuration_t**")
        if self.__config_path != None:
            configuration_file_name = ffi.new("char[]", os.fsencode(self.__config_path))
            error = lib.multio_new_configuration_from_filename(config, configuration_file_name)
            print(error)
        else:
            lib.multio_new_configuration(config)

        # Set free function
        self.config_pointer = ffi.gc(config[0], lib.multio_delete_configuration)

        if allow_world is not None:
            self.mpi_allow_world_default_comm(allow_world)

        if parent_comm is not None:
            self.mpi_parent_comm(parent_comm)

        if client_comm is not None:
            self.mpi_return_client_comm(client_comm)

        if server_comm is not None:
            self.mpi_return_server_comm(server_comm)

    def mpi_allow_world_default_comm(self, allow=0):

        multio_allow = ffi.cast("_Bool", allow)
        lib.multio_conf_mpi_allow_world_default_comm(self.config_pointer, multio_allow)

    def mpi_parent_comm(self, parent_comm=0):

        multio_par_comm = ffi.cast("int", parent_comm)
        lib.multio_conf_mpi_parent_comm(self.config_pointer, multio_par_comm)

    def mpi_return_client_comm(self, return_client_comm):

        multio_rcc = ffi.new("int[]", return_client_comm)
        lib.multio_conf_mpi_return_client_comm(self.config_pointer, multio_rcc)

    def mpi_return_server_comm(self, return_server_comm):

        multio_rsc = ffi.new("int[]", return_server_comm)
        lib.multio_conf_mpi_return_client_comm(self.config_pointer, multio_rsc)

class Multio:
    """
    This is the main interface class for Multio that users will interact with. It takes in
    a config file abd creates a multio handle, through this class a user can write data
    and interact with the multio c api.

    Parameters:
        config_path(str|file): A file-like object to where a plan is found. If not provided MULTIO_SERVER_CONFIG_FILE is checked
        allow_world(bool): Overwrite global MPI options for default splitting.
        parent_comm(array): Set MPI specific initalization parameters for parent comm.
        client_comm(array): Set MPI specific initalization parameters for client comm.
        server_comm(array): Set MPI specific initalization parameters for server comm.

    """

    def __init__(self, config_path=None, allow_world=None, parent_comm=None, client_comm=None, server_comm=None):

        self.__conf = _Config(config_path=config_path, allow_world=allow_world, parent_comm=parent_comm, client_comm=client_comm, server_comm=server_comm)

        handle = ffi.new("multio_handle_t**")
        lib.multio_new_handle(handle, self.__conf.config_pointer)

        self.__handle = ffi.gc(handle[0], lib.multio_delete_handle)

    def __enter__(self):
        lib.multio_open_connections(self.__handle)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        lib.multio_close_connections(self.__handle)

    def __version__(self):
        tmp_str = ffi.new("char**")
        lib.multio_version(tmp_str)
        versionstr = ffi.string(tmp_str[0]).decode("utf-8")
        return versionstr

    def start_server(self):
        lib.multio_start_server(self.__conf)

    def _create_metadata(self, md=None):
        return Metadata(self.__handle, md=md)

    def flush(self, md):
        """
        Indicates all servers that a given step is complete
        """
        if md is not None:
            lib.multio_flush(self.__handle, self._create_metadata(md).get_pointer())
        else:
            raise AttributeError(f"No metadata object instantiated")

    def notify(self, md):
        """
        Notifies all servers (e.g. step notification)
        and potentially performs triggers on sinks.
        """
        if md is not None:
            lib.multio_notify(self.__handle, self._create_metadata(md).get_pointer())
        else:
            raise AttributeError(f"No metadata object instantiated")

    def write_domain(self, md, data):
        """
        Writes domain information (e.g. local-to-global index mapping) to the server
        Parameters:
            data(array): Data of a single type usable by multio in the form an array 
        """
        if md is not None:
            data = ffi.new(f'int[{len(data)}]', data)
            size = ffi.cast("int", len(data))
            lib.multio_write_domain(self.__handle, self._create_metadata(md).get_pointer(), data, len(data))
        else:
            raise AttributeError(f"No metadata object instantiated")

    def write_mask(self, md, data):
        """
        Writes masking information (e.g. land-sea mask) to the server
        Parameters:
            data(array): Data of a single type usable by multio in the form an array 
        """
        if md is not None:
            data = ffi.new(f'float[{len(data)}]', data)
            size = ffi.cast("int", len(data))
            lib.multio_write_mask_float(self.__handle, self._create_metadata(md).get_pointer(), data, len(data))
        else:
            raise AttributeError(f"No metadata object instantiated")

    def write_field(self, md, data):
        """
        Writes (partial) fields
        Parameters:
            data(array): Data of a single type usable by multio in the form an array 
        """
        if md is not None:
            data = ffi.new(f'float[{len(data)}]', data)
            size = ffi.cast("int", len(data))
            lib.multio_write_field_float(self.__handle, self._create_metadata(md).get_pointer(), data, len(data))
        else:
            raise AttributeError(f"No metadata object instantiated")

    def field_accepted(self, md):
        """
        Determines if the pipelines are configured to accept the specified data

        Returns:
            boolean with True if accepted, otherwise False
        """
        if md is not None:
            accepted = False
            accept = ffi.new("bool*", accepted)
            lib.multio_field_accepted(self.__handle, self._create_metadata(md).get_pointer(), accept)
            return bool(accept[0])
        else:
            raise AttributeError(f"No metadata object instantiated")
