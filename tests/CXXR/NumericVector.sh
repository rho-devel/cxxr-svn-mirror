#!/bin/bash

R_HOME_DIR="../../"
R_HOME="${R_HOME_DIR}"
export R_HOME
R_SHARE_DIR="{$R_HOME_DIR}/share"
export R_SHARE_DIR
R_INCLUDE_DIR="${R_HOME_DIR}/include"
export R_INCLUDE_DIR
R_DOC_DIR="${R_HOME_DIR}/doc"
export R_DOC_DIR

. ${R_HOME}/etc/ldpaths

./NumericVector
