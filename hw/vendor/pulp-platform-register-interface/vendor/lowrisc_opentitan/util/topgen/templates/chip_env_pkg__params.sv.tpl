// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

// Generated by topgen.py

parameter string LIST_OF_ALERTS[] = {
% for alert in top["alert"]:
  % if loop.last:
  "${alert["name"]}"
  % else:
  "${alert["name"]}",
  % endif
% endfor
};

parameter uint NUM_ALERTS = ${len(top["alert"])};
