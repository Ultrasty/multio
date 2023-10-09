import multiopython

conf_dict = {
      "allow_world" : True,
      "parent_comm" : 1,
      "client_comm" : [2,3],
      "server_comm" : [4,5]
}

conf = multiopython.Config(**conf_dict)

#conf = multiopython.Config(allow_world=True, parent_comm=1, client_comm=[2,3], server_comm=[4,5])

handle = multiopython.Handler(conf)

metadata = {'category' : 'path',
      'new' : 1,
      'new_float' : 1.0,
      'trigger' : 'step',
      'step': 1}

md = multiopython.Metadata(handle, metadata)
md.metadata_set_string('category', 'path')
md.metadata_set_int('globalSize', 4)
md.metadata_set_int('level', 1)
md.metadata_set_int('step', 1)
md.metadata_set_string('trigger', 'step')
md.metadata_set_double('missingValue', 0.0)
md.metadata_set_bool('bitmapPresent', False)
md.metadata_set_int('bitsPerValue', 16)
md.metadata_set_bool('toAllServers', False)
md.metadata_set_string('name', 'test')


handle.write_field_double(md, [1.0,2.0,3.0,4.0], 4)
handle.notify(md)
handle.flush(md)
handle.field_accepted(md, False)

multio_object = multiopython.Multio(conf_dict)
multio_object.create_metadata(md=metadata)

multio_object.open_connections()
multio_object.write_field([1.0, 2.0, 3.0, 4.0])
multio_object.flush()
multio_object.notify()
multio_object.field_accepted(False)

multio_object.close_connections()
