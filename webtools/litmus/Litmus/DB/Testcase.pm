# -*- mode: cperl; c-basic-offset: 8; indent-tabs-mode: nil; -*-

=head1 COPYRIGHT

 # ***** BEGIN LICENSE BLOCK *****
 # Version: MPL 1.1
 #
 # The contents of this file are subject to the Mozilla Public License
 # Version 1.1 (the "License"); you may not use this file except in
 # compliance with the License. You may obtain a copy of the License
 # at http://www.mozilla.org/MPL/
 #
 # Software distributed under the License is distributed on an "AS IS"
 # basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 # the License for the specific language governing rights and
 # limitations under the License.
 #
 # The Original Code is Litmus.
 #
 # The Initial Developer of the Original Code is
 # the Mozilla Corporation.
 # Portions created by the Initial Developer are Copyright (C) 2006
 # the Initial Developer. All Rights Reserved.
 #
 # Contributor(s):
 #   Chris Cooper <ccooper@deadsquid.com>
 #   Zach Lipton <zach@zachlipton.com>
 #
 # ***** END LICENSE BLOCK *****

=cut

package Litmus::DB::Testcase;

use strict;
use base 'Litmus::DBI';

use Date::Manip;
#use Litmus::DB::Testresult;
#use Litmus::Error;

our $default_relevance_threshold = 1.0;
our $default_match_limit = 25;
our $default_num_days = 7;

Litmus::DB::Testcase->table('testcases');

Litmus::DB::Testcase->columns(Primary => qw/testcase_id/);
Litmus::DB::Testcase->columns(Essential => qw/summary details enabled community_enabled format_id regression_bug_id product_id steps expected_results author_id creation_date last_updated version testrunner_case_id testrunner_case_version branch_id/);
Litmus::DB::Testcase->columns(TEMP => qw /relevance subgroup_name/);

Litmus::DB::Testcase->column_alias("testcase_id", "testid");
Litmus::DB::Testcase->column_alias("testcase_id", "test_id");
Litmus::DB::Testcase->column_alias("testcase_id", "id");
Litmus::DB::Testcase->column_alias("community_enabled", "communityenabled");
Litmus::DB::Testcase->column_alias("format_id", "format");
Litmus::DB::Testcase->column_alias("author_id", "author");
Litmus::DB::Testcase->column_alias("product_id", "product");
Litmus::DB::Testcase->column_alias("branch_id", "branch");

Litmus::DB::Testcase->has_a("format" => "Litmus::DB::Format");
Litmus::DB::Testcase->has_a("author" => "Litmus::DB::User");
Litmus::DB::Testcase->has_a("product" => "Litmus::DB::Product");
Litmus::DB::Testcase->has_a("branch" => "Litmus::DB::Branch");

Litmus::DB::Testcase->has_many(test_results => "Litmus::DB::Testresult", {order_by => 'submission_time DESC'});

__PACKAGE__->set_sql(BySubgroup => qq{
SELECT t.* 
FROM testcases t, testcase_subgroups tsg
WHERE 
  tsg.subgroup_id=? AND
  tsg.testcase_id=t.testcase_id
  ORDER BY tsg.sort_order ASC
});

__PACKAGE__->set_sql(ByTestgroup => qq{
SELECT t.*, sg.name as subgroup_name
FROM testcases t, testcase_subgroups tsg, subgroup_testgroups sgtg, subgroups sg
WHERE 
  tsg.testcase_id=t.testcase_id AND
  tsg.subgroup_id=sgtg.subgroup_id AND
  tsg.subgroup_id = sg.subgroup_id AND
  sgtg.testgroup_id = ?
  ORDER BY sgtg.sort_order ASC, tsg.sort_order ASC
});

__PACKAGE__->set_sql(EnabledBySubgroup => qq{
SELECT DISTINCT(t.testcase_id),t.* 
FROM testcases t, testcase_subgroups tsg, subgroup_testgroups sgtg
WHERE 
  tsg.testcase_id=t.testcase_id AND
  tsg.subgroup_id=sgtg.subgroup_id AND
  tsg.subgroup_id=? AND 
  sgtg.testgroup_id=? AND
  t.enabled=1 
  ORDER BY tsg.sort_order ASC
});

__PACKAGE__->set_sql(CommunityEnabledBySubgroup => qq{
SELECT DISTINCT(t.testcase_id),t.*
FROM testcases t, testcase_subgroups tsg, subgroup_testgroups sgtg
WHERE 
  tsg.testcase_id=t.testcase_id AND
  tsg.subgroup_id=sgtg.subgroup_id AND
  tsg.subgroup_id=? AND 
  sgtg.testgroup_id=? AND
  t.enabled=1 AND 
  t.community_enabled=1
ORDER BY tsg.sort_order ASC
});

__PACKAGE__->set_sql(EnabledByTestRun => qq{
SELECT DISTINCT(tc.testcase_id)
FROM test_runs tr, test_run_testgroups trtg, testgroups tg, subgroup_testgroups sgtg, subgroups sg, testcase_subgroups tcsg, testcases tc
WHERE 
  tr.test_run_id=? AND 
  tr.test_run_id=trtg.test_run_id AND 
  trtg.testgroup_id=sgtg.testgroup_id AND 
  trtg.testgroup_id=tg.testgroup_id AND 
  sgtg.subgroup_id=tcsg.subgroup_id AND 
  sgtg.subgroup_id=sg.subgroup_id AND 
  tcsg.testcase_id=tc.testcase_id AND 
  tg.enabled=1 AND 
  sg.enabled=1 AND 
  tc.enabled=1
});

__PACKAGE__->set_sql(CommunityEnabledByTestRun => qq{
SELECT DISTINCT(tc.testcase_id)
FROM test_runs tr, test_run_testgroups trtg, testgroups tg, subgroup_testgroups sgtg, subgroups sg, testcase_subgroups tcsg, testcases tc
WHERE 
  tr.test_run_id=? AND 
  tr.test_run_id=trtg.test_run_id AND 
  trtg.testgroup_id=sgtg.testgroup_id AND 
  trtg.testgroup_id=tg.testgroup_id AND 
  sgtg.subgroup_id=tcsg.subgroup_id AND 
  sgtg.subgroup_id=sg.subgroup_id AND 
  tcsg.testcase_id=tc.testcase_id AND 
  tg.enabled=1 AND 
  sg.enabled=1 AND 
  tc.enabled=1 AND
  tc.community_enabled=1
});

__PACKAGE__->set_sql(Ungrouped => qq{
SELECT tc.*
FROM testcases tc LEFT JOIN testcase_subgroups tcsg ON (tc.testcase_id=tcsg.testcase_id)
WHERE tcsg.subgroup_id IS NULL
});

#########################################################################
# isCompleted($$$$$)
#
# Check whether we have test results for the current test that correspond
# to the provided platform, build_id, and user(optional).
#########################################################################
sub isCompleted {
  my $self = shift;
  my $opsys_id = shift;
  my $build_id = shift;
  my $locale = shift;
  my $user = shift;        # optional
  my $trusted = shift;        # optional

  my @results;
  if ($trusted) {
    @results = Litmus::DB::Testresult->search_CompletedByTrusted(
                                                                 $self->{'testcase_id'},
                                                                 $build_id,
                                                                 $locale,
                                                                 $opsys_id,
                                                                );
  } elsif ($user) {
    @results = Litmus::DB::Testresult->search_CompletedByUser(
                                                              $self->{'testcase_id'},
                                                              $build_id,
                                                              $locale,
                                                              $opsys_id,
                                                              $user->{'user_id'},
                                                             );
  } else {
    @results = Litmus::DB::Testresult->search_Completed(
                                                        $self->{'testcase_id'},
                                                        $build_id,
                                                        $locale,
                                                        $opsys_id,
                                                       );
  }

  return @results;
}

#########################################################################
sub coverage() {
    my $self = shift;
    my $test_run_id = shift;
    my $build_id = shift;
    my $platform_id = shift;
    my $opsys_id = shift;
    my $locale = shift;
    my $user = shift;        # optional
    my $trusted = shift;        # optional
 
    my $dbh = __PACKAGE__->db_Main();
    my $select = "SELECT tr.testresult_id, trsl.class_name";
    my $from = " FROM test_results tr, users u, opsyses o, test_result_status_lookup trsl";
    my $where = " WHERE tr.testcase_id=" . quotemeta($self->{'testcase_id'}) . " AND tr.user_id=u.user_id AND tr.opsys_id=o.opsys_id AND tr.result_status_id=trsl.result_status_id";
    my $order_by = " ORDER BY tr.submission_time DESC";

    if ($test_run_id) {
        $from .= ", test_runs trun, test_run_testgroups truntg, subgroup_testgroups sgtg, testcase_subgroups tcsg, testcases tc";
        $where .= " AND trun.test_run_id=" . quotemeta($test_run_id);
        $where .= " AND trun.test_run_id=truntg.test_run_id";
        $where .= " AND truntg.testgroup_id=sgtg.testgroup_id";
        $where .= " AND sgtg.subgroup_id=tcsg.subgroup_id";
        $where .= " AND tcsg.testcase_id=tc.testcase_id";
        $where .= " AND tc.testcase_id=tr.testcase_id";
        $where .= " AND tr.submission_time>=trun.start_timestamp";
        $where .= " AND tr.submission_time<trun.finish_timestamp";
        $where .= " AND trun.product_id=tc.product_id";
        $where .= " AND trun.branch_id=tr.branch_id";
    }

    if ($build_id) {
        $where .= " AND tr.build_id=" . quotemeta($build_id);
    }
    if ($platform_id) {
        $where .= " AND o.platform_id=" . quotemeta($platform_id);
    }
    if ($opsys_id) {
        $where .= " AND tr.opsys_id=" . quotemeta($opsys_id);
    }
    if ($locale) {
        $where .= " AND tr.locale_abbrev='" . quotemeta($locale) . "'";
    }
    if ($user) {
        $where .= " AND tr.user_id=" . quotemeta($user->{'user_id'});
    }
    if ($trusted) {
    	$from .= ", user_group_map ugm, security_groups sg";
        $where .= " AND tr.user_id=u.user_id AND u.user_id=ugm.user_id AND ugm.group_id=sg.group_id AND (sg.grouptype=1 OR sg.grouptype=3)";
    }
    my $sql = $select . $from . $where . $order_by;
    #print $sql,"<br/>\n";
    my $sth = $dbh->prepare_cached($sql);
    $sth->execute();
    my @test_results;
    while (my ($result_id, $class_name) = $sth->fetchrow_array) {
        my $hash_ref;
        $hash_ref->{'result_id'} = $result_id;
        $hash_ref->{'status'} = $class_name;
        push @test_results, $hash_ref;
    }
    $sth->finish;

    if (scalar(@test_results) == 0) {
        return undef;
    }

    return \@test_results;
}

#########################################################################
sub getFullTextMatches() {
  my $self = shift;
  my $text_snippet = shift;
  my $match_limit = shift;
  my $relevance_threshold = shift;
  
  if (!$match_limit) {
    $match_limit = $default_match_limit;
  }
  if (!$relevance_threshold) {
    $relevance_threshold = $default_relevance_threshold
  }
  
  __PACKAGE__->set_sql(FullTextMatches => qq{
                                             SELECT testcase_id, summary, creation_date, last_updated, MATCH (summary,steps,expected_results) AGAINST (?) AS relevance
					     FROM testcases
					     WHERE MATCH (summary,steps,expected_results) AGAINST (?) HAVING relevance > ?
					     ORDER BY relevance DESC, summary ASC
                                             LIMIT $match_limit
});
  
  
  return $self->search_FullTextMatches(
                                       $text_snippet,
                                       $text_snippet,
                                       $relevance_threshold
                                      );
}

#########################################################################
sub getNewTestcases() {
  my $self = shift;
  my $num_days = shift;
  my $match_limit = shift;
  
  if (!$num_days) {
    $num_days = $default_num_days;
  }
  
  if (!$match_limit) {
    $match_limit = $default_match_limit;
  }
  
  __PACKAGE__->set_sql(NewTestcases => qq{
				          SELECT testcase_id, summary, creation_date, last_updated
                                          FROM testcases
                                          WHERE creation_date>=?
                                          ORDER BY creation_date DESC
                                          LIMIT $match_limit
});
  
  my $err;
  my $new_datestamp=&UnixDate(DateCalc("now","- $num_days days"),"%q");
  return $self->search_NewTestcases($new_datestamp);
}

#########################################################################
sub getRecentlyUpdated() {
  my $self = shift;
  my $num_days = shift;
  my $match_limit = shift;
  
  if (!$num_days) {
    $num_days = $default_num_days;
  }
  
  if (!$match_limit) {
    $match_limit = $default_match_limit;
  }
  
  __PACKAGE__->set_sql(RecentlyUpdated => qq{
                                             SELECT testcase_id, summary, creation_date, last_updated
                                             FROM testcases
                                             WHERE last_updated>=? AND last_updated>creation_date
                                             ORDER BY last_updated DESC
                                             LIMIT $match_limit
});
  
  my $err;
  my $new_datestamp=&UnixDate(DateCalc("now","- $num_days days"),"%q");
  return $self->search_RecentlyUpdated($new_datestamp);
}

#########################################################################
sub getDefaultMatchLimit() {
  return $default_match_limit;
}

#########################################################################
sub getDefaultRelevanceThreshold() {
  return $default_relevance_threshold;
}

#########################################################################
sub getDefaultNumDays() {
  return $default_num_days;
}

#########################################################################
sub clone() {
  my $self = shift;
  
  my $new_testcase = $self->copy;
  if (!$new_testcase) {
    return undef;
  }
  
  # Update dates to now.
  my $now = &UnixDate("today","%q");
  $new_testcase->creation_date($now);
  $new_testcase->last_updated($now);
  $new_testcase->update();
  
  # Propagate subgroup membership;
  my $dbh = __PACKAGE__->db_Main();
  my $sql = "INSERT INTO testcase_subgroups (testcase_id,subgroup_id,sort_order) SELECT ?,subgroup_id,sort_order FROM testcase_subgroups WHERE testcase_id=?";
  
  my $rows = $dbh->do($sql,
		      undef,
		      $new_testcase->testcase_id,
		      $self->testcase_id
		     );
  if (! $rows) {
    # XXX: Do we need to throw a warning here?
    # What happens when we clone a testcase that doesn't belong to
    # any subgroups?
  }
  
  $sql = "INSERT INTO related_testcases (testcase_id, related_testcase_id) VALUES (?,?)";
  $rows = $dbh->do($sql, 
                   undef,
                   $self->testcase_id,
                   $new_testcase->testcase_id
                  );
  if (! $rows) {
    # XXX: Do we need to throw a warning here?
  }
  
  return $new_testcase;
}

#########################################################################
sub delete_from_subgroups() {
  my $self = shift;
  
  my $dbh = __PACKAGE__->db_Main();
  my $sql = "DELETE from testcase_subgroups WHERE testcase_id=?";
  return $dbh->do($sql,
                  undef,
                  $self->testcase_id
                 );
}

#########################################################################
sub delete_from_subgroup() {
  my $self = shift;
  my $subgroup_id = shift;  

  my $dbh = __PACKAGE__->db_Main();
  my $sql = "DELETE from testcase_subgroups WHERE testcase_id=? AND subgroup_id=?";
  return $dbh->do($sql,
                  undef,
                  $self->testcase_id,
                  $subgroup_id
                 );
}

#########################################################################
sub delete_from_related() {
  my $self = shift;
  
  my $dbh = __PACKAGE__->db_Main();
  my $sql = "DELETE from related_testcases WHERE testcase_id=? OR related_testcase_id=?";
  return $dbh->do($sql,
                  undef,
                  $self->testcase_id,
                  $self->testcase_id
                 );
}

#########################################################################
sub delete_with_refs() {
  my $self = shift;
  $self->delete_from_subgroups();
  $self->delete_from_related();
  return $self->delete;
}

#########################################################################
sub update_subgroups() {
  my $self = shift;
  my $new_subgroup_ids = shift;

  # We always want to delete the existing subgroups. 
  # Failing to delete subgroups is _not_ fatal when adding a new testcase.
  my $rv = $self->delete_from_subgroups();
  
  if (scalar @$new_subgroup_ids) {
    my $dbh = __PACKAGE__->db_Main();
    my $sql = "INSERT INTO testcase_subgroups (testcase_id,subgroup_id,sort_order) VALUES (?,?,1)";
    foreach my $new_subgroup_id (@$new_subgroup_ids) {
      my $rows = $dbh->do($sql, 
			  undef,
			  $self->testcase_id,
			  $new_subgroup_id
			 );
    }
  }
}

#########################################################################
sub update_subgroup() {
  my $self = shift;
  my $subgroup_id = shift;
  my $sort_order = shift;

  # Sort order defaults to 1.
  if (!$sort_order) {
    $sort_order = 1;
  }
  
  my $rv = $self->delete_from_subgroup($subgroup_id);
  my $dbh = __PACKAGE__->db_Main();
  my $sql = "INSERT INTO testcase_subgroups (testcase_id,subgroup_id,sort_order) VALUES (?,?,?)";
  return $dbh->do($sql, 
                  undef,
                  $self->testcase_id,
                  $subgroup_id,
                  $sort_order
                 );
}

1;
