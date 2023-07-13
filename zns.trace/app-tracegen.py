#! /usr/bin/python3

import sys, getopt
import os
import re
import json
import jsonpickle
import glob

from util.timeline import Timeline
from util.event import Event, MetaEvent
from util.helpers import *

JSON_FILE_NAME = ""

watch_inodes = dict()

timeline = Timeline()

def parse_inodes(file):
    for line in file:
        data = json.loads(line)
        for map_name, map_data in data["data"].items():
            if 'probes' in map_name:
                continue
            for key, value in map_data.items():
                watch_inodes[key] = value
 
def main(argv):
    try:
        opts, args = getopt.getopt(argv,"hd:",["data_file="])
    except getopt.GetoptError:
        print('Error. Usage: python3 timeline.py -d [relative path to json file of trace data]')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('Error. Usage: python3 tracegen.py -d [relative path to json file of trace data]')
            sys.exit()
        elif opt in ("-d", "--data_file"):
            global JSON_FILE_NAME
            JSON_FILE_NAME = arg

def parse_f2fs_and_vfs_probe_data(file):
    for line in file:
        data = json.loads(line)
        for map_name, map_data in data["data"].items():
            if 'probes' in map_name:
                continue
            for key, value in map_data.items():
                args = dict()
                items = key.split(",")
                timestamp = items[0]
                pid = items[1]
                tid = items[2]
                hint = 0
                inode = -1

                map_name = re.sub("@", "", map_name)
                if 'rw_hint' in map_name:
                    args["inode"] = str(value[0])
                    inode = str(value[0])
                    args["rw_hint"] = get_hint(int(value[1]))
                elif 'f2fs_submit_page_write' in map_name:
                    val = list(value)
                    args["inode"] = str(items[3])
                    inode = str(items[3])
                    args["LBA"] = int(val[0])
                    args["zone"] = int(val[1])
                    args["temp"] = get_temp(int(val[2]))
                    args["type"] = get_type(int(val[3]))
                else:
                    args["inode"] = str(value)
                    inode = str(value)

                if inode not in watch_inodes.keys():
                    continue

                args["file"] = watch_inodes[args["inode"]]

                event = Event(map_name, timestamp, "i", pid, tid, args)

                timeline.addTimestamp(event)

def parse_nvme_probe_data(file):
    for line in file:
        data = json.loads(line)
        for map_name, map_data in data["data"].items():
            if 'probes' in map_name:
                continue
            for key, value in map_data.items():
                args = dict()
                items = key.split(",")
                timestamp = items[0]
                pid = items[1]
                tid = items[2]

                map_name = re.sub("@", "", map_name)
                vals = list(value)

                args["cmd"] = get_cmd(vals[0])
                args["zone"] = vals[1] # only applies to zns, otherwise it will be 0
                args["LBA"] = vals[2]
                if 'nvme_rq' in map_name:
                    args["size"] = str(int(vals[3] * 512) / 1024) + "KiB" # TODO: add variable for block size
                    time = vals[4]
                else:
                    time = vals[3]
                
                event = Event(map_name, timestamp, "B", pid, tid, args)
                event_end = Event(map_name, time, "E", pid, tid, args)

                timeline.addTimestamp(event)
                timeline.addTimestamp(event_end)

def parse_rocksdb_probe_data(file):
    for line in file:
        data = json.loads(line)
        for map_name, map_data in data["data"].items():
            if 'probes' in map_name:
                continue
            for key, value in map_data.items():
                args = dict()
                items = key.split(",")
                timestamp = items[0]
                pid = items[1]
                tid = items[2]

                map_name = re.sub("@", "", map_name)

                # TODO: maybe we can figure out filename and involved files in compaction?
                # args["cmd"] = get_cmd(vals[0])
                # args["zone"] = vals[1] # only applies to zns, otherwise it will be 0
                # args["LBA"] = vals[2]
                endtime = int(timestamp) + int(value)
                
                event = Event(map_name, timestamp, "B", pid, tid, args)
                event_end = Event(map_name, endtime, "E", pid, tid, args)

                timeline.addTimestamp(event)
                timeline.addTimestamp(event_end)

def set_metadata_events():
    # Metadata event to change pid names to stack layers
    args = dict()
    args["name"] = "NVMe"
    event = MetaEvent("process_name", "M", 0, 0, args)
    timeline.addTimestamp(event)

    args = dict()
    args["name"] = "RocksDB"
    event = MetaEvent("process_name", "M", 1, 1, args)
    timeline.addTimestamp(event)

    args = dict()
    args["name"] = "VFS"
    event = MetaEvent("process_name", "M", 2, 2, args)
    timeline.addTimestamp(event)

    args = dict()
    args["name"] = "F2FS"
    event = MetaEvent("process_name", "M", 3, 3, args)
    timeline.addTimestamp(event)

    args = dict()
    args["name"] = "MM"
    event = MetaEvent("process_name", "M", 4, 4, args)
    timeline.addTimestamp(event)

    # set the process sorting order in timeline. seems to be a bug they won't address:
    # https://github.com/google/perfetto/issues/378
    args = dict()
    args["sort_index"] = 0
    event = MetaEvent("process_sort_index", "M", 1, 1, args)
    timeline.addTimestamp(event)
    args = dict()
    args["sort_index"] = 1
    event = MetaEvent("process_sort_index", "M", 2, 2, args)
    timeline.addTimestamp(event)
    args = dict()
    args["sort_index"] = 2
    event = MetaEvent("process_sort_index", "M", 3, 3, args)
    timeline.addTimestamp(event)
    args = dict()
    args["sort_index"] = 3
    event = MetaEvent("process_sort_index", "M", 4, 4, args)
    timeline.addTimestamp(event)
    args = dict()
    args["sort_index"] = 4
    event = MetaEvent("process_sort_index", "M", 0, 0, args)
    timeline.addTimestamp(event)


if __name__ == "__main__":
    main(sys.argv[1:])
    file_path = '/'.join(os.path.abspath(__file__).split('/')[:-1])

    with open(f"{file_path}/data/inodes.json") as file:
        parse_inodes(file)

    set_metadata_events()

    # TODO: we want to have different dirs for different traces coming from different times, parse a flag to specify which dir to use
    for file in glob.glob(f"{file_path}/data/*"):
        file_name = file.split('/')[-1]

        # existing timeline.json file for this data dir, overwrite it
        if 'timeline' in file_name:
            continue

        if 'inodes' in file_name:
            continue

        with open(f"{file_path}/data/{file_name}") as file:
            if 'f2fs' in file_name or 'vfs' in file_name or 'mm' in file_name:
                parse_f2fs_and_vfs_probe_data(file)
            elif 'nvme' in file_name:
                parse_nvme_probe_data(file)
            elif 'rocksdb' in file_name:
                parse_rocksdb_probe_data(file)
                        
    json_timeline = jsonpickle.encode(timeline, unpicklable=False, keys=True)

    with open(f"{file_path}/data/timeline.json", 'w') as outfile:
        outfile.write(json_timeline + '\n')
