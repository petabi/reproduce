# Changelog

This file documents all notable changes to this project. The format of this file
is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this
project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased] 2019-06-28

### Added

- `-d` specifies the datasource id. `Default datasource id = 1`
- `-n` specifies the `prefix` of data file name. This option is
 useful when the `-i` option is DIRECTORY and you want to send
 only the files that the name begin with the specified prefix.

### Changed

- When a DIRECTORY is specified for input, reproduce will send files in `alphabetical order`.
- When a DIRECTORY is specified for input, reproduce will send the files in it's `sub-directories`.
- The `event id` will start at 1 rather than 0.
- Current `Time` will be printed in the report file.
