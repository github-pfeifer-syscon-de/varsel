/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2024 RPf <gpl3@pfeifer-syscon.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <clocale>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <git2.h>
#include <giomm.h>

#include "GitRepository.hpp"
#include "Git_test.hpp"
//#include "common.h"
//#include "args.h"

Git_test::Git_test()
{
    //struct opts o = { 1, 0, 0, 0, GIT_REPOSITORY_INIT_SHARED_UMASK, 0, 0, 0 };

	git_libgit2_init();
	//parse_opts(&o, 0, nullptr);
}

//typedef struct {
//    int val;
//} status_data;

//
//enum {
//	FORMAT_DEFAULT   = 0,
//	FORMAT_LONG      = 1,
//	FORMAT_SHORT     = 2,
//	FORMAT_PORCELAIN = 3
//};
//
//#define MAX_PATHSPEC 8
//
//struct status_opts {
//	git_status_options statusopt;
//	char *repodir;
//	char *pathspec[MAX_PATHSPEC];
//	int npaths;
//	int format;
//	int zterm;
//	int showbranch;
//	int showsubmod;
//	int repeat;
//};
//
//static void parse_opts(struct status_opts *o, int argc, char *argv[]);
//static void show_branch(git_repository *repo, int format);
//static void print_long(git_status_list *status);
//static void print_short(git_repository *repo, git_status_list *status);
//static int print_submod(git_submodule *sm, const char *name, void *payload);
//
//static void
//check_lg2(int error, const char* msg, const char* ctx)
//{
//    if (error != 0) {
//        fprintf(stderr, "%s %s %s\n",
//                msg, (ctx != nullptr ? ctx : ""), git_error_last()->message);
//    }
//}
//
//static void
//fatal(const char* msg, const char* ctx)
//{
//    fprintf(stderr, "%s %s Unable to continue\n",
//            msg, (ctx != nullptr ? ctx : ""));
//    exit(1);
//}
//
///**
// * This version of the output prefixes each path with two status
// * columns and shows submodule status information.
// */
//static void
//print_short(git_repository *repo, git_status_list *status)
//{
//	size_t i, maxi = git_status_list_entrycount(status);
//	const git_status_entry *s;
//	char istatus, wstatus;
//	const char *extra, *a, *b, *c;
//
//	for (i = 0; i < maxi; ++i) {
//		s = git_status_byindex(status, i);
//
//		if (s->status == GIT_STATUS_CURRENT)
//			continue;
//
//		a = b = c = NULL;
//		istatus = wstatus = ' ';
//		extra = "";
//
//		if (s->status & GIT_STATUS_INDEX_NEW)
//			istatus = 'A';
//		if (s->status & GIT_STATUS_INDEX_MODIFIED)
//			istatus = 'M';
//		if (s->status & GIT_STATUS_INDEX_DELETED)
//			istatus = 'D';
//		if (s->status & GIT_STATUS_INDEX_RENAMED)
//			istatus = 'R';
//		if (s->status & GIT_STATUS_INDEX_TYPECHANGE)
//			istatus = 'T';
//
//		if (s->status & GIT_STATUS_WT_NEW) {
//			if (istatus == ' ')
//				istatus = '?';
//			wstatus = '?';
//		}
//		if (s->status & GIT_STATUS_WT_MODIFIED)
//			wstatus = 'M';
//		if (s->status & GIT_STATUS_WT_DELETED)
//			wstatus = 'D';
//		if (s->status & GIT_STATUS_WT_RENAMED)
//			wstatus = 'R';
//		if (s->status & GIT_STATUS_WT_TYPECHANGE)
//			wstatus = 'T';
//
//		if (s->status & GIT_STATUS_IGNORED) {
//			istatus = '!';
//			wstatus = '!';
//		}
//
//		if (istatus == '?' && wstatus == '?')
//			continue;
//
//		/**
//		 * A commit in a tree is how submodules are stored, so
//		 * let's go take a look at its status.
//		 */
//		if (s->index_to_workdir &&
//			s->index_to_workdir->new_file.mode == GIT_FILEMODE_COMMIT)
//		{
//			unsigned int smstatus = 0;
//
//			if (!git_submodule_status(&smstatus, repo, s->index_to_workdir->new_file.path,
//						  GIT_SUBMODULE_IGNORE_UNSPECIFIED)) {
//				if (smstatus & GIT_SUBMODULE_STATUS_WD_MODIFIED)
//					extra = " (new commits)";
//				else if (smstatus & GIT_SUBMODULE_STATUS_WD_INDEX_MODIFIED)
//					extra = " (modified content)";
//				else if (smstatus & GIT_SUBMODULE_STATUS_WD_WD_MODIFIED)
//					extra = " (modified content)";
//				else if (smstatus & GIT_SUBMODULE_STATUS_WD_UNTRACKED)
//					extra = " (untracked content)";
//			}
//		}
//
//		/**
//		 * Now that we have all the information, format the output.
//		 */
//
//		if (s->head_to_index) {
//			a = s->head_to_index->old_file.path;
//			b = s->head_to_index->new_file.path;
//		}
//		if (s->index_to_workdir) {
//			if (!a)
//				a = s->index_to_workdir->old_file.path;
//			if (!b)
//				b = s->index_to_workdir->old_file.path;
//			c = s->index_to_workdir->new_file.path;
//		}
//
//		if (istatus == 'R') {
//			if (wstatus == 'R')
//				printf("%c%c %s %s %s%s\n", istatus, wstatus, a, b, c, extra);
//			else
//				printf("%c%c %s %s%s\n", istatus, wstatus, a, b, extra);
//		} else {
//			if (wstatus == 'R')
//				printf("%c%c %s %s%s\n", istatus, wstatus, a, c, extra);
//			else
//				printf("%c%c %s%s\n", istatus, wstatus, a, extra);
//		}
//	}
//
//	for (i = 0; i < maxi; ++i) {
//		s = git_status_byindex(status, i);
//
//		if (s->status == GIT_STATUS_WT_NEW)
//			printf("?? %s\n", s->index_to_workdir->old_file.path);
//	}
//}
//
////
////static int
////status_cb(const char *path,
////              unsigned int status_flags,
////              void *payload)
////{
////  status_data *d = (status_data*)payload;
////
////  printf("status %s %x\n", path, status_flags);
////
////  return 0;
////}
//
///**
// * If the user asked for the branch, let's show the short name of the
// * branch.
// */
//static void show_branch(git_repository *repo, int format)
//{
//	int error = 0;
//	const char *branch = NULL;
//	git_reference *head = NULL;
//
//	error = git_repository_head(&head, repo);
//
//	if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND)
//		branch = NULL;
//	else if (!error) {
//		branch = git_reference_shorthand(head);
//	} else
//		check_lg2(error, "failed to get current branch", NULL);
//
//	if (format == FORMAT_LONG)
//		printf("# On branch %s\n",
//			branch ? branch : "Not currently on any branch.");
//	else
//		printf("## %s\n", branch ? branch : "HEAD (no branch)");
//
//	git_reference_free(head);
//}
//
///**
// * This function print out an output similar to git's status command
// * in long form, including the command-line hints.
// */
//static void print_long(git_status_list *status)
//{
//	size_t i, maxi = git_status_list_entrycount(status);
//	const git_status_entry *s;
//	int header = 0, changes_in_index = 0;
//	int changed_in_workdir = 0, rm_in_workdir = 0;
//	const char *old_path, *new_path;
//
//	/** Print index changes. */
//
//	for (i = 0; i < maxi; ++i) {
//		const char *istatus = NULL;
//
//		s = git_status_byindex(status, i);
//
//		if (s->status == GIT_STATUS_CURRENT)
//			continue;
//
//		if (s->status & GIT_STATUS_WT_DELETED)
//			rm_in_workdir = 1;
//
//		if (s->status & GIT_STATUS_INDEX_NEW)
//			istatus = "new file: ";
//		if (s->status & GIT_STATUS_INDEX_MODIFIED)
//			istatus = "modified: ";
//		if (s->status & GIT_STATUS_INDEX_DELETED)
//			istatus = "deleted:  ";
//		if (s->status & GIT_STATUS_INDEX_RENAMED)
//			istatus = "renamed:  ";
//		if (s->status & GIT_STATUS_INDEX_TYPECHANGE)
//			istatus = "typechange:";
//
//		if (istatus == NULL)
//			continue;
//
//		if (!header) {
//			printf("# Changes to be committed:\n");
//			printf("#   (use \"git reset HEAD <file>...\" to unstage)\n");
//			printf("#\n");
//			header = 1;
//		}
//
//		old_path = s->head_to_index->old_file.path;
//		new_path = s->head_to_index->new_file.path;
//
//		if (old_path && new_path && strcmp(old_path, new_path))
//			printf("#\t%s  %s -> %s\n", istatus, old_path, new_path);
//		else
//			printf("#\t%s  %s\n", istatus, old_path ? old_path : new_path);
//	}
//
//	if (header) {
//		changes_in_index = 1;
//		printf("#\n");
//	}
//	header = 0;
//
//	/** Print workdir changes to tracked files. */
//
//	for (i = 0; i < maxi; ++i) {
//		const char *wstatus = NULL;
//
//		s = git_status_byindex(status, i);
//
//		/**
//		 * With `GIT_STATUS_OPT_INCLUDE_UNMODIFIED` (not used in this example)
//		 * `index_to_workdir` may not be `NULL` even if there are
//		 * no differences, in which case it will be a `GIT_DELTA_UNMODIFIED`.
//		 */
//		if (s->status == GIT_STATUS_CURRENT || s->index_to_workdir == NULL)
//			continue;
//
//		/** Print out the output since we know the file has some changes */
//		if (s->status & GIT_STATUS_WT_MODIFIED)
//			wstatus = "modified: ";
//		if (s->status & GIT_STATUS_WT_DELETED)
//			wstatus = "deleted:  ";
//		if (s->status & GIT_STATUS_WT_RENAMED)
//			wstatus = "renamed:  ";
//		if (s->status & GIT_STATUS_WT_TYPECHANGE)
//			wstatus = "typechange:";
//
//		if (wstatus == NULL)
//			continue;
//
//		if (!header) {
//			printf("# Changes not staged for commit:\n");
//			printf("#   (use \"git add%s <file>...\" to update what will be committed)\n", rm_in_workdir ? "/rm" : "");
//			printf("#   (use \"git checkout -- <file>...\" to discard changes in working directory)\n");
//			printf("#\n");
//			header = 1;
//		}
//
//		old_path = s->index_to_workdir->old_file.path;
//		new_path = s->index_to_workdir->new_file.path;
//
//		if (old_path && new_path && strcmp(old_path, new_path))
//			printf("#\t%s  %s -> %s\n", wstatus, old_path, new_path);
//		else
//			printf("#\t%s  %s\n", wstatus, old_path ? old_path : new_path);
//	}
//
//	if (header) {
//		changed_in_workdir = 1;
//		printf("#\n");
//	}
//
//	/** Print untracked files. */
//
//	header = 0;
//
//	for (i = 0; i < maxi; ++i) {
//		s = git_status_byindex(status, i);
//
//		if (s->status == GIT_STATUS_WT_NEW) {
//
//			if (!header) {
//				printf("# Untracked files:\n");
//				printf("#   (use \"git add <file>...\" to include in what will be committed)\n");
//				printf("#\n");
//				header = 1;
//			}
//
//			printf("#\t%s\n", s->index_to_workdir->old_file.path);
//		}
//	}
//
//	header = 0;
//
//	/** Print ignored files. */
//
//	for (i = 0; i < maxi; ++i) {
//		s = git_status_byindex(status, i);
//
//		if (s->status == GIT_STATUS_IGNORED) {
//
//			if (!header) {
//				printf("# Ignored files:\n");
//				printf("#   (use \"git add -f <file>...\" to include in what will be committed)\n");
//				printf("#\n");
//				header = 1;
//			}
//
//			printf("#\t%s\n", s->index_to_workdir->old_file.path);
//		}
//	}
//
//	if (!changes_in_index && changed_in_workdir)
//		printf("no changes added to commit (use \"git add\" and/or \"git commit -a\")\n");
//}
//
//
//static int print_submod(git_submodule *sm, const char *name, void *payload)
//{
//	int *count = reinterpret_cast<int*>(payload);
//	(void)name;
//
//	if (*count == 0)
//		printf("# Submodules\n");
//	(*count)++;
//
//	printf("# - submodule '%s' at %s\n",
//		git_submodule_name(sm), git_submodule_path(sm));
//
//	return 0;
//}
//
///**
// * Parse options that git's status command supports.
// */
//static void parse_opts(struct status_opts *o, int argc, char *argv[])
//{
//	struct args_info args = ARGS_INFO_INIT;
//
//	for (args.pos = 1; args.pos < argc; ++args.pos) {
//		char *a = argv[args.pos];
//
//		if (a[0] != '-') {
//			if (o->npaths < MAX_PATHSPEC)
//				o->pathspec[o->npaths++] = a;
//			else
//				fatal("Example only supports a limited pathspec", NULL);
//		}
//		else if (!strcmp(a, "-s") || !strcmp(a, "--short"))
//			o->format = FORMAT_SHORT;
//		else if (!strcmp(a, "--long"))
//			o->format = FORMAT_LONG;
//		else if (!strcmp(a, "--porcelain"))
//			o->format = FORMAT_PORCELAIN;
//		else if (!strcmp(a, "-b") || !strcmp(a, "--branch"))
//			o->showbranch = 1;
//		else if (!strcmp(a, "-z")) {
//			o->zterm = 1;
//			if (o->format == FORMAT_DEFAULT)
//				o->format = FORMAT_PORCELAIN;
//		}
//		else if (!strcmp(a, "--ignored"))
//			o->statusopt.flags |= GIT_STATUS_OPT_INCLUDE_IGNORED;
//		else if (!strcmp(a, "-uno") ||
//				 !strcmp(a, "--untracked-files=no"))
//			o->statusopt.flags &= ~GIT_STATUS_OPT_INCLUDE_UNTRACKED;
//		else if (!strcmp(a, "-unormal") ||
//				 !strcmp(a, "--untracked-files=normal"))
//			o->statusopt.flags |= GIT_STATUS_OPT_INCLUDE_UNTRACKED;
//		else if (!strcmp(a, "-uall") ||
//				 !strcmp(a, "--untracked-files=all"))
//			o->statusopt.flags |= GIT_STATUS_OPT_INCLUDE_UNTRACKED |
//				GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
//		else if (!strcmp(a, "--ignore-submodules=all"))
//			o->statusopt.flags |= GIT_STATUS_OPT_EXCLUDE_SUBMODULES;
//		else if (!strncmp(a, "--git-dir=", strlen("--git-dir=")))
//			o->repodir = a + strlen("--git-dir=");
//		else if (!strcmp(a, "--repeat"))
//			o->repeat = 10;
//		//else if (match_int_arg(&o->repeat, &args, "--repeat", 0))
//			///* okay */;
//		else if (!strcmp(a, "--list-submodules"))
//			o->showsubmod = 1;
//		else
//			check_lg2(-1, "Unsupported option", a);
//	}
//
//	if (o->format == FORMAT_DEFAULT)
//		o->format = FORMAT_LONG;
//	if (o->format == FORMAT_LONG)
//		o->showbranch = 1;
//	if (o->npaths > 0) {
//		o->statusopt.pathspec.strings = o->pathspec;
//		o->statusopt.pathspec.count   = o->npaths;
//	}
//}
//
//
//bool
//Git_test::reposC(int argc, char** argv)
//{
//    std::cout << "Git_test::reposC ----------" << std::endl;
//    // Open a repository
//    git_repository *repo{nullptr};
//    int error = git_repository_open(&repo, "/home/rpf/csrc.git/genericImg");
//    if (error < 0) {
//        fprintf(stderr, "Could not open repository: %s\n", git_error_last()->message);
//        return false;
//    }
//
//    // Dereference HEAD to a commit
//    git_object *head_commit;
//    error = git_revparse_single(&head_commit, repo, "HEAD^{commit}");   //
//    if (error != 0) {
//        fprintf(stderr, "No head commit: %s\n", git_error_last()->message);
//        return false;
//    }
//    git_commit *commit = (git_commit*)head_commit;
//
//    // Print some of the commit's properties
//    printf("%s\n", git_commit_message(commit));
//    const git_signature *author = git_commit_author(commit);
//    if (author) {
//        printf("    %s <%s>\n",
//                 author->name ? author->name : "?"
//               , author->email ? author->email: "?");
//    }
//    else {
//        printf("No author\n");
//    }
//
//    git_status_list *status;
//	struct status_opts o{};
//    o.statusopt = GIT_STATUS_OPTIONS_INIT;
//    o.repodir = ".";
//    parse_opts(&o, argc, argv);
//
//	o.statusopt.show  = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
//	o.statusopt.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
//		GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
//		GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;
//
//    check_lg2(git_status_list_new(&status, repo, &o.statusopt),
//        "Error status", o.repodir);
//
//    if (o.showbranch)
//		show_branch(repo, o.format);
//
//	if (o.showsubmod) {
//		int submod_count = 0;
//		check_lg2(git_submodule_foreach(repo, print_submod, &submod_count),
//			"Cannot iterate submodules", o.repodir);
//	}
//
//	if (o.format == FORMAT_LONG)
//		print_long(status);
//	else
//		print_short(repo, status);
//
//	git_status_list_free(status);
//
//
//    // Cleanup
//    git_commit_free(commit);
//    git_repository_free(repo);
//    return true;
//}

bool
Git_test::reposCpp()
{
    std::cout << "Git_test::reposCpp ----------" << std::endl;
    try {
        psc::git::Repository repo("/home/rpf/csrc.git/genericImg");
        auto commit = repo.getSingelCommit("HEAD^{commit}");
        std::cout << commit->getMessage()  << std::endl;
        auto sig = commit->getSignature();
        std::cout << "    " << sig->getAuthor() << " <" << sig->getEmail() << ">" << std::endl;
        std::cout << "    " << sig->getWhen().format_iso8601() << std::endl;

        std::cout << "On branch " << repo.getBranch() << std::endl;
        std::cout << "Status\tIndex\t\t\tWorkdir" << std::endl;
        auto status = repo.getStatus();
        for (auto iter = status.begin(); iter != status.end(); ++iter) {
            auto stat = *iter;
            std::cout << stat.to_string() << std::endl;
        }
        auto iters = repo.getIter("refs/heads/*");
        std::cout << "Iterator refs/heads/*" << std::endl;
        for (auto iter : iters) {
            std::cout << iter << std::endl;
        }
        auto remote = repo.getRemote("origin");
        std::cout << "Remote name: " << remote.getName() << "\n"
                  << "   url: " << remote.getUrl() << "\n"
                  << "   pushUrl: " << remote.getPushUrl() << std::endl;
    }
    catch (const psc::git::GitException& exc) {
        std::cout << "Error " << exc.what() << std::endl;
        return false;
    }
    auto home = Glib::get_home_dir();
    std::cout << "home " << home << std::endl;
    return true;
}

bool
Git_test::test_file()
{
    std::cout << "Git_test::test_file ----------- " << std::endl;
    auto file = Gio::File::create_for_path("/abc/def/srcöäü/test.xyz");
    if (file->has_parent()) {
        std::cout << "parse " << file->get_parse_name() << std::endl;
        std::cout << "path " << file->get_path() << std::endl;
        std::cout << "base " << file->get_basename() << std::endl;
        std::cout << "parent " << file->get_parent()->get_path() << std::endl;
    }
    else {
        std::cout << "no parent" << std::endl;
    }
    return true;
}

int main(int argc, char** argv)
{
    std::setlocale(LC_ALL, "");      // make locale dependent, and make glib accept u8 const !!!
    Glib::init();
    Git_test git_test;
    if (!git_test.reposCpp()) {
        return 2;
    }
    if (!git_test.test_file()) {
        return 3;
    }


    return 0;
}

