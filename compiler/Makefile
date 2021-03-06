# Copyright 2004-2017 Cray Inc.
# Other additional copyright holders may be indicated within.
#
# The entirety of this work is licensed under the Apache License,
# Version 2.0 (the "License"); you may not use this file except
# in compliance with the License.
#
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# Makefile: builds Chapel compiler
#

ifndef CHPL_MAKE_HOME
export CHPL_MAKE_HOME=$(shell pwd)/..
endif


COMPILER_ROOT=.
COMPILER_SUBDIR = .

#
# subdirectories and false subdirectory-oriented targets to force recursion
#
SUBDIRS = \
	adt \
	AST \
	backend \
	codegen \
	ifa \
	ipe \
	main \
	optimizations \
	parser \
	passes \
	resolution \
	util \

#
# include standard header for compiler
#
include $(COMPILER_ROOT)/make/Makefile.compiler.head

# Generate tags command, dependent on if Make variable, TAGS == 1
ifeq ($(TAGS), 1)
TAGS_COMMAND=-@(which $(CHPL_TAGS_UTIL) > /dev/null 2>&1 && echo "Updating TAGS..." && $(CHPL_TAGS_UTIL) $(CHPL_TAGS_FLAGS) $(ALL_SRCS) */*.h) || echo "$(CHPL_TAGS_UTIL) not available"
EBROWSE_COMMAND=-@(which ebrowse > /dev/null 2>&1 && echo "Updating BROWSE..." && ebrowse $(ALL_SRCS) */*.h) || echo "ebrowse not available"
endif

#
# the location of the BUILD_VERSION file
#
BUILD_VERSION_FILE = $(COMPILER_ROOT)/main/BUILD_VERSION
# configured prefix file
CONFIGURED_PREFIX_FILE = $(COMPILER_ROOT)/main/CONFIGURED_PREFIX



#
# include source subdirectories here
#
include adt/Makefile.include
include AST/Makefile.include
include backend/Makefile.include
include codegen/Makefile.include
include ifa/Makefile.include
include ipe/Makefile.include
include main/Makefile.include
include optimizations/Makefile.include
include parser/Makefile.include
include passes/Makefile.include
include resolution/Makefile.include
include util/Makefile.include

SVN_SRCS =

CHPL_OBJS = \
	$(ADT_OBJS) \
	$(AST_OBJS) \
	$(BACKEND_OBJS) \
	$(CODEGEN_OBJS) \
	$(IFA_OBJS) \
	$(IPE_OBJS) \
	$(MAIN_OBJS) \
	$(OPTIMIZATIONS_OBJS) \
	$(PARSER_OBJS) \
	$(PASSES_OBJS) \
	$(RESOLUTION_OBJS) \
	$(SYMTAB_OBJS) \
	$(UTIL_OBJS) \

EXECS = $(CHPL) $(CHPLDOC) $(CHPLIPE)

PRETARGETS = $(BUILD_VERSION_FILE) $(CONFIGURED_PREFIX_FILE) llvm
TARGETS = $(CHPL)


#
# main rules
#

include $(COMPILER_ROOT)/make/Makefile.compiler.rules


#
# target-based rules
#

CHPL_CONFIG_CHECK_PREFIX = $(CHPL_BIN_DIR)/.built-for
CHPL_CONFIG_CHECK_DIR = $(CHPL_CONFIG_CHECK_PREFIX)/$(CHPL_MAKE_COMPILER_SUBDIR)
CHPL_CONFIG_CHECK = $(CHPL_CONFIG_CHECK_DIR)/built-for

UPDATE_BUILD_VERSION = $(CHPL_MAKE_HOME)/util/devel/updateBuildVersion

$(BUILD_VERSION_FILE): FORCE
	@({ test -x $(CHPL_MAKE_HOME)/.git ; } && \
	test -x $(UPDATE_BUILD_VERSION) && $(UPDATE_BUILD_VERSION) $@) || \
	test -r $(BUILD_VERSION_FILE) || (echo '"0"' > $@);

$(CHPL_MAKE_HOME)/configured-prefix:
	echo > $(CHPL_MAKE_HOME)/configured-prefix

$(CONFIGURED_PREFIX_FILE): $(CHPL_MAKE_HOME)/configured-prefix
	echo '"'`cat $(CHPL_MAKE_HOME)/configured-prefix`'"' \ > $(CONFIGURED_PREFIX_FILE)

$(CHPL_CONFIG_CHECK): | $(CHPL_BIN_DIR)
	rm -rf $(CHPL_CONFIG_CHECK_PREFIX)
	mkdir -p $(CHPL_CONFIG_CHECK_DIR)
	echo $(CHPL_MAKE_COMPILER_SUBDIR) > $(CHPL_CONFIG_CHECK)


$(CHPL): $(CHPL_OBJS) $(CHPL_CONFIG_CHECK) | $(CHPL_BIN_DIR)
	$(CXX) $(LDFLAGS) -o $@ $(CHPL_OBJS) $(LIBS) $(LLVM_LIBS)
	rm -f $(CHPLIPE)
	ln -s $(notdir $(CHPL)) $(CHPLIPE)
	$(TAGS_COMMAND)
	$(EBROWSE_COMMAND)

$(CHPLDOC): $(CHPL)
	rm -f $(CHPLDOC)
	ln -s $(notdir $(CHPL)) $(CHPLDOC)

chpldoc: $(CHPLDOC)

parser: FORCE
	$(MAKE) -C parser parser

$(CHPL_BIN_DIR):
	mkdir -p $@

# third-party pkgs could be automatically built with the below.
#
#THIRD_PARTY_PKGS = $(shell $(CHPL_MAKE_HOME)/util/chplenv/third-party-pkgs --compiler)
# NOTE: If any of these builds fail, we won't stop building
#third-party-pkgs:
#	@for p in $(THIRD_PARTY_PKGS); do \
#		$(MAKE) -C $(THIRD_PARTY_DIR) $$p; \
#	done

llvm: FORCE
	$(MAKE) -C $(THIRD_PARTY_DIR)/llvm $(CHPL_MAKE_LLVM)

#
# include standard footer for compiler
#
include $(COMPILER_ROOT)/make/Makefile.compiler.foot

.NOTPARALLEL:
