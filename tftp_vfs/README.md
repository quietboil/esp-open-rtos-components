# TFTP Virtual File System

This module provides a switchboard for TFTP file handlers. This allows creating multiple TFTP contexts, one per file if needed, as separate modules.

The workflow is actually very trivial. This module provides TFTP context to LWIP TFTP server. When TFTP server is asked to read or write a file this module will query all registered VFS in the order they were registered. The first one that responds with an open file handle becomes the one responsible for the rest of the IO.

One of the interesting usages for this, and the real reason it was created in the first place :wink:, is ability to report internal state of various components - sort of a /proc file system for ESP.

## Usage

See [OTA Update](../ota_update) for an example of creating a VFS, registering it with this modules and what is required to build a program with it and how to initialize the VFS.