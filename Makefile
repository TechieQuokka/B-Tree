# B-Tree 라이브러리 Makefile
# 크로스 플랫폼 지원 (Linux, macOS, Windows/MinGW)

# 프로젝트 설정
PROJECT_NAME := btree
VERSION := 1.0.0
LIB_NAME := lib$(PROJECT_NAME)

# 디렉터리 구조
SRCDIR := src
INCDIR := include
TESTDIR := tests
EXAMPLEDIR := examples
BUILDDIR := build
LIBDIR := $(BUILDDIR)/lib
OBJDIR := $(BUILDDIR)/obj
BINDIR := $(BUILDDIR)/bin
DOCDIR := docs

# 소스 파일
SOURCES := $(wildcard $(SRCDIR)/*.c)
HEADERS := $(wildcard $(INCDIR)/*.h)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# 테스트 및 예제 파일
TEST_SOURCES := $(wildcard $(TESTDIR)/*.c)
TEST_TARGETS := $(TEST_SOURCES:$(TESTDIR)/%.c=$(BINDIR)/%)

EXAMPLE_SOURCES := $(wildcard $(EXAMPLEDIR)/*.c)
EXAMPLE_TARGETS := $(EXAMPLE_SOURCES:$(EXAMPLEDIR)/%.c=$(BINDIR)/%)

# 컴파일러 및 플래그 설정
CC := gcc
AR := ar
RANLIB := ranlib

# 기본 컴파일 플래그
CFLAGS := -std=c99 -Wall -Wextra -Wpedantic -I$(INCDIR)
LDFLAGS := 
LIBS := -lm

# 플랫폼 감지
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(UNAME_S),Linux)
    PLATFORM := linux
    SHARED_EXT := .so
    STATIC_EXT := .a
    CFLAGS += -fPIC -D_GNU_SOURCE
    LIBS += -lpthread
endif

ifeq ($(UNAME_S),Darwin)
    PLATFORM := macos
    SHARED_EXT := .dylib
    STATIC_EXT := .a
    CFLAGS += -fPIC
    LIBS += -lpthread
endif

ifneq (,$(findstring MINGW,$(UNAME_S)))
    PLATFORM := windows
    SHARED_EXT := .dll
    STATIC_EXT := .lib
    LIBS += -lws2_32
endif

# 빌드 모드 설정
ifeq ($(MODE),debug)
    CFLAGS += -g -O0 -DBTREE_DEBUG -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS += -fsanitize=address
    BUILDDIR := $(BUILDDIR)/debug
else ifeq ($(MODE),release)
    CFLAGS += -O3 -DNDEBUG -flto
    LDFLAGS += -flto
    BUILDDIR := $(BUILDDIR)/release
else
    CFLAGS += -O2 -g
    BUILDDIR := $(BUILDDIR)/default
endif

# 특징 플래그
ifdef ENABLE_NUMA
    CFLAGS += -DBTREE_NUMA_SUPPORT
    LIBS += -lnuma
endif

ifdef ENABLE_THREADING
    CFLAGS += -DBTREE_THREAD_SAFE
    LIBS += -lpthread
endif

ifdef ENABLE_COMPRESSION
    CFLAGS += -DBTREE_COMPRESSION_SUPPORT
    LIBS += -lz -llz4
endif

# 라이브러리 타겟
STATIC_LIB := $(LIBDIR)/$(LIB_NAME)$(STATIC_EXT)
SHARED_LIB := $(LIBDIR)/$(LIB_NAME)$(SHARED_EXT)

# 디렉터리 재정의 (빌드 모드 적용)
LIBDIR := $(BUILDDIR)/lib
OBJDIR := $(BUILDDIR)/obj
BINDIR := $(BUILDDIR)/bin

# 기본 타겟
.PHONY: all clean test examples install uninstall docs help
.DEFAULT_GOAL := all

all: static shared

# 정적 라이브러리 빌드
static: $(STATIC_LIB)

$(STATIC_LIB): $(OBJECTS) | $(LIBDIR)
	@echo "정적 라이브러리 생성: $@"
	$(AR) rcs $@ $(OBJECTS)
	$(RANLIB) $@

# 공유 라이브러리 빌드
shared: $(SHARED_LIB)

$(SHARED_LIB): $(OBJECTS) | $(LIBDIR)
	@echo "공유 라이브러리 생성: $@"
ifeq ($(PLATFORM),macos)
	$(CC) -shared -Wl,-install_name,$(notdir $@) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)
else
	$(CC) -shared $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)
endif

# 오브젝트 파일 컴파일
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS) | $(OBJDIR)
	@echo "컴파일: $<"
	$(CC) $(CFLAGS) -c $< -o $@

# 테스트 빌드 및 실행
test: $(BINDIR)/test_btree
	@echo "테스트 실행..."
	$(BINDIR)/test_btree

$(BINDIR)/test_btree: $(TESTDIR)/test_btree.c $(STATIC_LIB) | $(BINDIR)
	@echo "테스트 빌드: $@"
	$(CC) $(CFLAGS) $< -L$(LIBDIR) -l$(PROJECT_NAME) $(LDFLAGS) $(LIBS) -o $@

# 성능 테스트
perf-test: $(BINDIR)/test_btree
	@echo "성능 테스트 실행..."
	$(BINDIR)/test_btree --perf

# 예제 빌드
examples: $(EXAMPLE_TARGETS)

$(BINDIR)/%: $(EXAMPLEDIR)/%.c $(STATIC_LIB) | $(BINDIR)
	@echo "예제 빌드: $@"
	$(CC) $(CFLAGS) $< -L$(LIBDIR) -l$(PROJECT_NAME) $(LDFLAGS) $(LIBS) -o $@

# 벤치마크 실행
benchmark: $(BINDIR)/basic_usage
	@echo "벤치마크 실행..."
	$(BINDIR)/basic_usage

# 디렉터리 생성
$(OBJDIR) $(LIBDIR) $(BINDIR):
	@mkdir -p $@

# 정적 분석
analyze:
	@echo "정적 분석 실행..."
	@which cppcheck >/dev/null 2>&1 && cppcheck --enable=all --inconclusive $(SRCDIR) $(INCDIR) || echo "cppcheck가 설치되지 않았습니다"
	@which clang-tidy >/dev/null 2>&1 && clang-tidy $(SOURCES) -- $(CFLAGS) || echo "clang-tidy가 설치되지 않았습니다"

# 메모리 누수 검사 (Valgrind)
memcheck: $(BINDIR)/test_btree
	@echo "메모리 누수 검사..."
	@which valgrind >/dev/null 2>&1 && valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(BINDIR)/test_btree || echo "valgrind가 설치되지 않았습니다"

# 코드 포매팅
format:
	@echo "코드 포매팅..."
	@which clang-format >/dev/null 2>&1 && find $(SRCDIR) $(INCDIR) $(TESTDIR) $(EXAMPLEDIR) -name "*.c" -o -name "*.h" | xargs clang-format -i || echo "clang-format이 설치되지 않았습니다"

# 문서 생성
docs:
	@echo "문서 생성..."
	@which doxygen >/dev/null 2>&1 && doxygen Doxyfile || echo "doxygen이 설치되지 않았습니다"

# 설치
install: all
	@echo "라이브러리 설치..."
	@install -d $(DESTDIR)/usr/local/lib
	@install -d $(DESTDIR)/usr/local/include
	@install -m 644 $(STATIC_LIB) $(DESTDIR)/usr/local/lib/
ifneq ($(PLATFORM),windows)
	@install -m 755 $(SHARED_LIB) $(DESTDIR)/usr/local/lib/
endif
	@install -m 644 $(HEADERS) $(DESTDIR)/usr/local/include/
	@echo "설치 완료"

# 제거
uninstall:
	@echo "라이브러리 제거..."
	@rm -f $(DESTDIR)/usr/local/lib/$(notdir $(STATIC_LIB))
	@rm -f $(DESTDIR)/usr/local/lib/$(notdir $(SHARED_LIB))
	@rm -f $(DESTDIR)/usr/local/include/btree*.h
	@echo "제거 완료"

# 패키지 생성
package: all
	@echo "패키지 생성..."
	@mkdir -p $(BUILDDIR)/package/$(PROJECT_NAME)-$(VERSION)
	@cp -r $(INCDIR) $(BUILDDIR)/package/$(PROJECT_NAME)-$(VERSION)/
	@cp -r $(LIBDIR) $(BUILDDIR)/package/$(PROJECT_NAME)-$(VERSION)/
	@cp -r $(EXAMPLEDIR) $(BUILDDIR)/package/$(PROJECT_NAME)-$(VERSION)/
	@cp -r $(DOCDIR) $(BUILDDIR)/package/$(PROJECT_NAME)-$(VERSION)/
	@cp README.md LICENSE Makefile $(BUILDDIR)/package/$(PROJECT_NAME)-$(VERSION)/
	@cd $(BUILDDIR)/package && tar czf $(PROJECT_NAME)-$(VERSION)-$(PLATFORM).tar.gz $(PROJECT_NAME)-$(VERSION)
	@echo "패키지 생성 완료: $(BUILDDIR)/package/$(PROJECT_NAME)-$(VERSION)-$(PLATFORM).tar.gz"

# 정리
clean:
	@echo "빌드 파일 정리..."
	@rm -rf $(BUILDDIR)
	@echo "정리 완료"

# 전체 정리 (문서 포함)
distclean: clean
	@echo "전체 정리..."
	@rm -rf $(DOCDIR)/html $(DOCDIR)/latex
	@echo "전체 정리 완료"

# 빠른 테스트 (컴파일만)
check: $(OBJECTS)
	@echo "컴파일 검사 완료"

# 커버리지 테스트
coverage: CFLAGS += --coverage
coverage: LDFLAGS += --coverage
coverage: test
	@echo "커버리지 보고서 생성..."
	@which gcov >/dev/null 2>&1 && gcov $(SOURCES) || echo "gcov가 설치되지 않았습니다"
	@which lcov >/dev/null 2>&1 && lcov --capture --directory $(OBJDIR) --output-file coverage.info && genhtml coverage.info --output-directory coverage-html || echo "lcov가 설치되지 않았습니다"

# 여러 설정으로 빌드
all-configs:
	@echo "모든 설정으로 빌드..."
	@$(MAKE) MODE=debug clean all test
	@$(MAKE) MODE=release clean all test
	@$(MAKE) MODE=debug ENABLE_THREADING=1 clean all test
	@$(MAKE) MODE=release ENABLE_NUMA=1 clean all test

# CI/CD용 빌드
ci: all test examples analyze
	@echo "CI 빌드 완료"

# 개발 환경 설정
dev-setup:
	@echo "개발 환경 설정..."
	@echo "필요한 도구들을 설치하세요:"
	@echo "  - gcc (또는 clang)"
	@echo "  - make"
	@echo "  - valgrind (메모리 검사용)"
	@echo "  - cppcheck (정적 분석용)"
	@echo "  - clang-format (코드 포매팅용)"
	@echo "  - doxygen (문서 생성용)"
	@echo "  - lcov (커버리지 측정용)"

# 도움말
help:
	@echo "B-Tree 라이브러리 빌드 시스템"
	@echo "사용법: make [타겟] [옵션]"
	@echo ""
	@echo "주요 타겟:"
	@echo "  all          - 정적 및 공유 라이브러리 빌드 (기본)"
	@echo "  static       - 정적 라이브러리만 빌드"
	@echo "  shared       - 공유 라이브러리만 빌드"
	@echo "  test         - 테스트 빌드 및 실행"
	@echo "  examples     - 예제 프로그램 빌드"
	@echo "  clean        - 빌드 파일 정리"
	@echo "  install      - 시스템에 라이브러리 설치"
	@echo "  package      - 배포용 패키지 생성"
	@echo ""
	@echo "개발 도구:"
	@echo "  analyze      - 정적 분석 실행"
	@echo "  memcheck     - 메모리 누수 검사"
	@echo "  coverage     - 코드 커버리지 측정"
	@echo "  format       - 코드 포매팅"
	@echo "  docs         - API 문서 생성"
	@echo ""
	@echo "빌드 옵션:"
	@echo "  MODE=debug|release|default"
	@echo "  ENABLE_NUMA=1         - NUMA 지원 활성화"
	@echo "  ENABLE_THREADING=1    - 스레딩 지원 활성화"
	@echo "  ENABLE_COMPRESSION=1  - 압축 지원 활성화"
	@echo ""
	@echo "예제:"
	@echo "  make MODE=debug test        - 디버그 모드로 테스트"
	@echo "  make ENABLE_NUMA=1 all      - NUMA 지원으로 빌드"
	@echo "  make MODE=release package   - 릴리스 패키지 생성"

# 의존성 정보 출력
info:
	@echo "빌드 정보:"
	@echo "  프로젝트:    $(PROJECT_NAME) v$(VERSION)"
	@echo "  플랫폼:      $(PLATFORM)"
	@echo "  컴파일러:    $(CC) $(shell $(CC) --version | head -1)"
	@echo "  빌드 모드:   $(if $(MODE),$(MODE),default)"
	@echo "  CFLAGS:      $(CFLAGS)"
	@echo "  LDFLAGS:     $(LDFLAGS)"
	@echo "  LIBS:        $(LIBS)"
	@echo "  빌드 디렉터리: $(BUILDDIR)"

# 빌드 의존성 체크
check-deps:
	@echo "의존성 체크..."
	@which $(CC) >/dev/null 2>&1 || (echo "오류: $(CC) 컴파일러가 없습니다" && exit 1)
	@which $(AR) >/dev/null 2>&1 || (echo "오류: $(AR) 아카이버가 없습니다" && exit 1)
	@echo "모든 필수 의존성이 설치되어 있습니다"

# 포함 의존성 생성 (자동 의존성 관리)
-include $(OBJECTS:.o=.d)

$(OBJDIR)/%.d: $(SRCDIR)/%.c | $(OBJDIR)
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OBJDIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$