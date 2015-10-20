


<!DOCTYPE html>
<html lang="en" class=" is-copy-enabled">
  <head prefix="og: http://ogp.me/ns# fb: http://ogp.me/ns/fb# object: http://ogp.me/ns/object# article: http://ogp.me/ns/article# profile: http://ogp.me/ns/profile#">
    <meta charset='utf-8'>
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta http-equiv="Content-Language" content="en">
    <meta name="viewport" content="width=1020">
    
    
    <title>osmc/common.sh at master · osmc/osmc</title>
    <link rel="search" type="application/opensearchdescription+xml" href="/opensearch.xml" title="GitHub">
    <link rel="fluid-icon" href="https://github.com/fluidicon.png" title="GitHub">
    <link rel="apple-touch-icon" sizes="57x57" href="/apple-touch-icon-114.png">
    <link rel="apple-touch-icon" sizes="114x114" href="/apple-touch-icon-114.png">
    <link rel="apple-touch-icon" sizes="72x72" href="/apple-touch-icon-144.png">
    <link rel="apple-touch-icon" sizes="144x144" href="/apple-touch-icon-144.png">
    <meta property="fb:app_id" content="1401488693436528">

      <meta content="@github" name="twitter:site" /><meta content="summary" name="twitter:card" /><meta content="osmc/osmc" name="twitter:title" /><meta content="osmc - OSMC (Open Source Media Center) is a free and open source media center distribution" name="twitter:description" /><meta content="https://avatars0.githubusercontent.com/u/9024675?v=3&amp;s=400" name="twitter:image:src" />
      <meta content="GitHub" property="og:site_name" /><meta content="object" property="og:type" /><meta content="https://avatars0.githubusercontent.com/u/9024675?v=3&amp;s=400" property="og:image" /><meta content="osmc/osmc" property="og:title" /><meta content="https://github.com/osmc/osmc" property="og:url" /><meta content="osmc - OSMC (Open Source Media Center) is a free and open source media center distribution" property="og:description" />
      <meta name="browser-stats-url" content="https://api.github.com/_private/browser/stats">
    <meta name="browser-errors-url" content="https://api.github.com/_private/browser/errors">
    <link rel="assets" href="https://assets-cdn.github.com/">
    <link rel="web-socket" href="wss://live.github.com/_sockets/MTUxOTMxMTU6ZTVmODMxMTI1YTE2MjVjNjYzMTQ3MWI5MWMwZDAwOWU6N2MxZTI3MDdkY2Y3ODQzNGVjYmJmY2Y3ZjEwMGZkZjk1YzI1MTdhY2U0MTBmZmNlMWJhNjAxMTJmOGU2YTk3YQ==--9f291e7b83c82f8a5171b2895e4a97905b7f40e7">
    <meta name="pjax-timeout" content="1000">
    <link rel="sudo-modal" href="/sessions/sudo_modal">

    <meta name="msapplication-TileImage" content="/windows-tile.png">
    <meta name="msapplication-TileColor" content="#ffffff">
    <meta name="selected-link" value="repo_source" data-pjax-transient>

    <meta name="google-site-verification" content="KT5gs8h0wvaagLKAVWq8bbeNwnZZK1r1XQysX3xurLU">
    <meta name="google-analytics" content="UA-3769691-2">

<meta content="collector.githubapp.com" name="octolytics-host" /><meta content="github" name="octolytics-app-id" /><meta content="57A831C0:7D26:1729AF80:56263D20" name="octolytics-dimension-request_id" /><meta content="15193115" name="octolytics-actor-id" /><meta content="mega-force" name="octolytics-actor-login" /><meta content="d7badb8a81c89b4a06774f44364fdacd229152be0b71b4974fbdab54ac2f1d96" name="octolytics-actor-hash" />

<meta content="Rails, view, blob#show" data-pjax-transient="true" name="analytics-event" />


  <meta class="js-ga-set" name="dimension1" content="Logged In">
    <meta class="js-ga-set" name="dimension4" content="Current repo nav">




    <meta name="is-dotcom" content="true">
        <meta name="hostname" content="github.com">
    <meta name="user-login" content="mega-force">

      <link rel="mask-icon" href="https://assets-cdn.github.com/pinned-octocat.svg" color="#4078c0">
      <link rel="icon" type="image/x-icon" href="https://assets-cdn.github.com/favicon.ico">

    <meta content="0dd74ad73d506abb6727e8282cd469479eef0f96" name="form-nonce" />

    <link crossorigin="anonymous" href="https://assets-cdn.github.com/assets/github-60348b331fa450b2228c6d7901d9afe1921385ec9f6f544047e84a4073d825c1.css" media="all" rel="stylesheet" />
    <link crossorigin="anonymous" href="https://assets-cdn.github.com/assets/github2-9e83ef1ecc7ceba3d66e3033dd980274ecac3fac0bc5b987cf82d42c663f6f8a.css" media="all" rel="stylesheet" />
    
    
    


    <meta http-equiv="x-pjax-version" content="ef4817c743444ed4205f254c8a1a8b49">

      
  <meta name="description" content="osmc - OSMC (Open Source Media Center) is a free and open source media center distribution">
  <meta name="go-import" content="github.com/osmc/osmc git https://github.com/osmc/osmc.git">

  <meta content="9024675" name="octolytics-dimension-user_id" /><meta content="osmc" name="octolytics-dimension-user_login" /><meta content="20389482" name="octolytics-dimension-repository_id" /><meta content="osmc/osmc" name="octolytics-dimension-repository_nwo" /><meta content="true" name="octolytics-dimension-repository_public" /><meta content="false" name="octolytics-dimension-repository_is_fork" /><meta content="20389482" name="octolytics-dimension-repository_network_root_id" /><meta content="osmc/osmc" name="octolytics-dimension-repository_network_root_nwo" />
  <link href="https://github.com/osmc/osmc/commits/master.atom" rel="alternate" title="Recent Commits to osmc:master" type="application/atom+xml">

  </head>


  <body class="logged_in   env-production linux vis-public page-blob">
    <a href="#start-of-content" tabindex="1" class="accessibility-aid js-skip-to-content">Skip to content</a>

    
    
    



      <div class="header header-logged-in true" role="banner">
  <div class="container clearfix">

    <a class="header-logo-invertocat" href="https://github.com/" data-hotkey="g d" aria-label="Homepage" data-ga-click="Header, go to dashboard, icon:logo">
  <span class="mega-octicon octicon-mark-github"></span>
</a>


      <div class="site-search repo-scope js-site-search" role="search">
          <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/osmc/osmc/search" class="js-site-search-form" data-global-search-url="/search" data-repo-search-url="/osmc/osmc/search" method="get"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /></div>
  <label class="js-chromeless-input-container form-control">
    <div class="scope-badge">This repository</div>
    <input type="text"
      class="js-site-search-focus js-site-search-field is-clearable chromeless-input"
      data-hotkey="s"
      name="q"
      placeholder="Search"
      aria-label="Search this repository"
      data-global-scope-placeholder="Search GitHub"
      data-repo-scope-placeholder="Search"
      tabindex="1"
      autocapitalize="off">
  </label>
</form>
      </div>

      <ul class="header-nav left" role="navigation">
        <li class="header-nav-item">
          <a href="/pulls" class="js-selected-navigation-item header-nav-link" data-ga-click="Header, click, Nav menu - item:pulls context:user" data-hotkey="g p" data-selected-links="/pulls /pulls/assigned /pulls/mentioned /pulls">
            Pull requests
</a>        </li>
        <li class="header-nav-item">
          <a href="/issues" class="js-selected-navigation-item header-nav-link" data-ga-click="Header, click, Nav menu - item:issues context:user" data-hotkey="g i" data-selected-links="/issues /issues/assigned /issues/mentioned /issues">
            Issues
</a>        </li>
          <li class="header-nav-item">
            <a class="header-nav-link" href="https://gist.github.com/" data-ga-click="Header, go to gist, text:gist">Gist</a>
          </li>
      </ul>

    
<ul class="header-nav user-nav right" id="user-links">
  <li class="header-nav-item">
      <span class="js-socket-channel js-updatable-content"
        data-channel="notification-changed:mega-force"
        data-url="/notifications/header">
      <a href="/notifications" aria-label="You have no unread notifications" class="header-nav-link notification-indicator tooltipped tooltipped-s" data-ga-click="Header, go to notifications, icon:read" data-hotkey="g n">
          <span class="mail-status all-read"></span>
          <span class="octicon octicon-bell"></span>
</a>  </span>

  </li>

  <li class="header-nav-item dropdown js-menu-container">
    <a class="header-nav-link tooltipped tooltipped-s js-menu-target" href="/new"
       aria-label="Create new…"
       data-ga-click="Header, create new, icon:add">
      <span class="octicon octicon-plus left"></span>
      <span class="dropdown-caret"></span>
    </a>

    <div class="dropdown-menu-content js-menu-content">
      <ul class="dropdown-menu dropdown-menu-sw">
        
<a class="dropdown-item" href="/new" data-ga-click="Header, create new repository">
  New repository
</a>


  <a class="dropdown-item" href="/organizations/new" data-ga-click="Header, create new organization">
    New organization
  </a>



  <div class="dropdown-divider"></div>
  <div class="dropdown-header">
    <span title="osmc/osmc">This repository</span>
  </div>
    <a class="dropdown-item" href="/osmc/osmc/issues/new" data-ga-click="Header, create new issue">
      New issue
    </a>

      </ul>
    </div>
  </li>

  <li class="header-nav-item dropdown js-menu-container">
    <a class="header-nav-link name tooltipped tooltipped-s js-menu-target" href="/mega-force"
       aria-label="View profile and more"
       data-ga-click="Header, show menu, icon:avatar">
      <img alt="@mega-force" class="avatar" height="20" src="https://avatars2.githubusercontent.com/u/15193115?v=3&amp;s=40" width="20" />
      <span class="dropdown-caret"></span>
    </a>

    <div class="dropdown-menu-content js-menu-content">
      <div class="dropdown-menu  dropdown-menu-sw">
        <div class=" dropdown-header header-nav-current-user css-truncate">
            Signed in as <strong class="css-truncate-target">mega-force</strong>

        </div>


        <div class="dropdown-divider"></div>

          <a class="dropdown-item" href="/mega-force" data-ga-click="Header, go to profile, text:your profile">
            Your profile
          </a>
        <a class="dropdown-item" href="/stars" data-ga-click="Header, go to starred repos, text:your stars">
          Your stars
        </a>
        <a class="dropdown-item" href="/explore" data-ga-click="Header, go to explore, text:explore">
          Explore
        </a>
          <a class="dropdown-item" href="/integrations" data-ga-click="Header, go to integrations, text:integrations">
            Integrations
          </a>
        <a class="dropdown-item" href="https://help.github.com" data-ga-click="Header, go to help, text:help">
          Help
        </a>

          <div class="dropdown-divider"></div>

          <a class="dropdown-item" href="/settings/profile" data-ga-click="Header, go to settings, icon:settings">
            Settings
          </a>

          <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/logout" class="logout-form" data-form-nonce="0dd74ad73d506abb6727e8282cd469479eef0f96" method="post"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /><input name="authenticity_token" type="hidden" value="JbQcb93p5OZ9hlbm00rOLZTIkWEc4m8J+mcTB+Ie6ypc+3u/bm66PRgLNpmat19FNUDXKCV1X90KROh5wIDrfw==" /></div>
            <button class="dropdown-item dropdown-signout" data-ga-click="Header, sign out, icon:logout">
              Sign out
            </button>
</form>
      </div>
    </div>
  </li>
</ul>


    
  </div>
</div>

      

      


    <div id="start-of-content" class="accessibility-aid"></div>

    <div id="js-flash-container">
</div>


    <div role="main" class="main-content">
        <div itemscope itemtype="http://schema.org/WebPage">
    <div class="pagehead repohead instapaper_ignore readability-menu">

      <div class="container">

        <div class="clearfix">
          

<ul class="pagehead-actions">

  <li>
      <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/notifications/subscribe" class="js-social-container" data-autosubmit="true" data-form-nonce="0dd74ad73d506abb6727e8282cd469479eef0f96" data-remote="true" method="post"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /><input name="authenticity_token" type="hidden" value="1rzNlzlHYPWgE9n1eYebcv3O4gYvRfTub2hwCK8+c2hZoGVm9cCGMWnkvr4gy00rbrrzyI9FGGJe+a1HZ8ZeOw==" /></div>    <input id="repository_id" name="repository_id" type="hidden" value="20389482" />

      <div class="select-menu js-menu-container js-select-menu">
        <a href="/osmc/osmc/subscription"
          class="btn btn-sm btn-with-count select-menu-button js-menu-target" role="button" tabindex="0" aria-haspopup="true"
          data-ga-click="Repository, click Watch settings, action:blob#show">
          <span class="js-select-button">
            <span class="octicon octicon-eye"></span>
            Watch
          </span>
        </a>
        <a class="social-count js-social-count" href="/osmc/osmc/watchers">
          98
        </a>

        <div class="select-menu-modal-holder">
          <div class="select-menu-modal subscription-menu-modal js-menu-content" aria-hidden="true">
            <div class="select-menu-header">
              <span class="select-menu-title">Notifications</span>
              <span class="octicon octicon-x js-menu-close" role="button" aria-label="Close"></span>
            </div>

            <div class="select-menu-list js-navigation-container" role="menu">

              <div class="select-menu-item js-navigation-item selected" role="menuitem" tabindex="0">
                <span class="select-menu-item-icon octicon octicon-check"></span>
                <div class="select-menu-item-text">
                  <input checked="checked" id="do_included" name="do" type="radio" value="included" />
                  <span class="select-menu-item-heading">Not watching</span>
                  <span class="description">Be notified when participating or @mentioned.</span>
                  <span class="js-select-button-text hidden-select-button-text">
                    <span class="octicon octicon-eye"></span>
                    Watch
                  </span>
                </div>
              </div>

              <div class="select-menu-item js-navigation-item " role="menuitem" tabindex="0">
                <span class="select-menu-item-icon octicon octicon octicon-check"></span>
                <div class="select-menu-item-text">
                  <input id="do_subscribed" name="do" type="radio" value="subscribed" />
                  <span class="select-menu-item-heading">Watching</span>
                  <span class="description">Be notified of all conversations.</span>
                  <span class="js-select-button-text hidden-select-button-text">
                    <span class="octicon octicon-eye"></span>
                    Unwatch
                  </span>
                </div>
              </div>

              <div class="select-menu-item js-navigation-item " role="menuitem" tabindex="0">
                <span class="select-menu-item-icon octicon octicon-check"></span>
                <div class="select-menu-item-text">
                  <input id="do_ignore" name="do" type="radio" value="ignore" />
                  <span class="select-menu-item-heading">Ignoring</span>
                  <span class="description">Never be notified.</span>
                  <span class="js-select-button-text hidden-select-button-text">
                    <span class="octicon octicon-mute"></span>
                    Stop ignoring
                  </span>
                </div>
              </div>

            </div>

          </div>
        </div>
      </div>
</form>
  </li>

  <li>
    
  <div class="js-toggler-container js-social-container starring-container ">

    <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/osmc/osmc/unstar" class="js-toggler-form starred js-unstar-button" data-form-nonce="0dd74ad73d506abb6727e8282cd469479eef0f96" data-remote="true" method="post"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /><input name="authenticity_token" type="hidden" value="52EtTo9noSiGECJwjmFo/4oRk6eVh1CK9k/S1Bf6Wh0/ON/1eHpnq/BWfD1GOEiJq9ZytSAmH/oBncVtYWKX4w==" /></div>
      <button
        class="btn btn-sm btn-with-count js-toggler-target"
        aria-label="Unstar this repository" title="Unstar osmc/osmc"
        data-ga-click="Repository, click unstar button, action:blob#show; text:Unstar">
        <span class="octicon octicon-star"></span>
        Unstar
      </button>
        <a class="social-count js-social-count" href="/osmc/osmc/stargazers">
          454
        </a>
</form>
    <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/osmc/osmc/star" class="js-toggler-form unstarred js-star-button" data-form-nonce="0dd74ad73d506abb6727e8282cd469479eef0f96" data-remote="true" method="post"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /><input name="authenticity_token" type="hidden" value="s+ybw1JLBPJPD22fGYYp9apctKi9A+PKqJG4akjZYj7yKCijtxUs/K2SHuJ71brTx14GP4212T1G0KwPkTZYYg==" /></div>
      <button
        class="btn btn-sm btn-with-count js-toggler-target"
        aria-label="Star this repository" title="Star osmc/osmc"
        data-ga-click="Repository, click star button, action:blob#show; text:Star">
        <span class="octicon octicon-star"></span>
        Star
      </button>
        <a class="social-count js-social-count" href="/osmc/osmc/stargazers">
          454
        </a>
</form>  </div>

  </li>

  <li>
          <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/osmc/osmc/fork" class="btn-with-count" data-form-nonce="0dd74ad73d506abb6727e8282cd469479eef0f96" method="post"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /><input name="authenticity_token" type="hidden" value="cFdY+2UOZGTyLU53s6xVH5+r0L/AzsRCFb8dNJBvCrmIi+v5x6cUGLzQ7yKi8RaIFINlwWyTCnAMvzyiPMvV/A==" /></div>
            <button
                type="submit"
                class="btn btn-sm btn-with-count"
                data-ga-click="Repository, show fork modal, action:blob#show; text:Fork"
                title="Fork your own copy of osmc/osmc to your account"
                aria-label="Fork your own copy of osmc/osmc to your account">
              <span class="octicon octicon-repo-forked"></span>
              Fork
            </button>
</form>
    <a href="/osmc/osmc/network" class="social-count">
      145
    </a>
  </li>
</ul>

          <h1 itemscope itemtype="http://data-vocabulary.org/Breadcrumb" class="entry-title public ">
  <span class="mega-octicon octicon-repo"></span>
  <span class="author"><a href="/osmc" class="url fn" itemprop="url" rel="author"><span itemprop="title">osmc</span></a></span><!--
--><span class="path-divider">/</span><!--
--><strong><a href="/osmc/osmc" data-pjax="#js-repo-pjax-container">osmc</a></strong>

  <span class="page-context-loader">
    <img alt="" height="16" src="https://assets-cdn.github.com/images/spinners/octocat-spinner-32.gif" width="16" />
  </span>

</h1>

        </div>
      </div>
    </div>

    <div class="container">
      <div class="repository-with-sidebar repo-container new-discussion-timeline ">
        <div class="repository-sidebar clearfix">
          
<nav class="sunken-menu repo-nav js-repo-nav js-sidenav-container-pjax js-octicon-loaders"
     role="navigation"
     data-pjax="#js-repo-pjax-container"
     data-issue-count-url="/osmc/osmc/issues/counts">
  <ul class="sunken-menu-group">
    <li class="tooltipped tooltipped-w" aria-label="Code">
      <a href="/osmc/osmc" aria-label="Code" aria-selected="true" class="js-selected-navigation-item selected sunken-menu-item" data-hotkey="g c" data-selected-links="repo_source repo_downloads repo_commits repo_releases repo_tags repo_branches /osmc/osmc">
        <span class="octicon octicon-code"></span> <span class="full-word">Code</span>
        <img alt="" class="mini-loader" height="16" src="https://assets-cdn.github.com/images/spinners/octocat-spinner-32.gif" width="16" />
</a>    </li>

      <li class="tooltipped tooltipped-w" aria-label="Issues">
        <a href="/osmc/osmc/issues" aria-label="Issues" class="js-selected-navigation-item sunken-menu-item" data-hotkey="g i" data-selected-links="repo_issues repo_labels repo_milestones /osmc/osmc/issues">
          <span class="octicon octicon-issue-opened"></span> <span class="full-word">Issues</span>
          <span class="js-issue-replace-counter"></span>
          <img alt="" class="mini-loader" height="16" src="https://assets-cdn.github.com/images/spinners/octocat-spinner-32.gif" width="16" />
</a>      </li>

    <li class="tooltipped tooltipped-w" aria-label="Pull requests">
      <a href="/osmc/osmc/pulls" aria-label="Pull requests" class="js-selected-navigation-item sunken-menu-item" data-hotkey="g p" data-selected-links="repo_pulls /osmc/osmc/pulls">
          <span class="octicon octicon-git-pull-request"></span> <span class="full-word">Pull requests</span>
          <span class="js-pull-replace-counter"></span>
          <img alt="" class="mini-loader" height="16" src="https://assets-cdn.github.com/images/spinners/octocat-spinner-32.gif" width="16" />
</a>    </li>

  </ul>
  <div class="sunken-menu-separator"></div>
  <ul class="sunken-menu-group">

    <li class="tooltipped tooltipped-w" aria-label="Pulse">
      <a href="/osmc/osmc/pulse" aria-label="Pulse" class="js-selected-navigation-item sunken-menu-item" data-selected-links="pulse /osmc/osmc/pulse">
        <span class="octicon octicon-pulse"></span> <span class="full-word">Pulse</span>
        <img alt="" class="mini-loader" height="16" src="https://assets-cdn.github.com/images/spinners/octocat-spinner-32.gif" width="16" />
</a>    </li>

    <li class="tooltipped tooltipped-w" aria-label="Graphs">
      <a href="/osmc/osmc/graphs" aria-label="Graphs" class="js-selected-navigation-item sunken-menu-item" data-selected-links="repo_graphs repo_contributors /osmc/osmc/graphs">
        <span class="octicon octicon-graph"></span> <span class="full-word">Graphs</span>
        <img alt="" class="mini-loader" height="16" src="https://assets-cdn.github.com/images/spinners/octocat-spinner-32.gif" width="16" />
</a>    </li>
  </ul>


</nav>

            <div class="only-with-full-nav">
                
<div class="js-clone-url clone-url open"
  data-protocol-type="http">
  <h3 class="text-small text-muted"><span class="text-emphasized">HTTPS</span> clone URL</h3>
  <div class="input-group js-zeroclipboard-container">
    <input type="text" class="input-mini text-small text-muted input-monospace js-url-field js-zeroclipboard-target"
           value="https://github.com/osmc/osmc.git" readonly="readonly" aria-label="HTTPS clone URL">
    <span class="input-group-button">
      <button aria-label="Copy to clipboard" class="js-zeroclipboard btn btn-sm zeroclipboard-button tooltipped tooltipped-s" data-copied-hint="Copied!" type="button"><span class="octicon octicon-clippy"></span></button>
    </span>
  </div>
</div>

  
<div class="js-clone-url clone-url "
  data-protocol-type="ssh">
  <h3 class="text-small text-muted"><span class="text-emphasized">SSH</span> clone URL</h3>
  <div class="input-group js-zeroclipboard-container">
    <input type="text" class="input-mini text-small text-muted input-monospace js-url-field js-zeroclipboard-target"
           value="git@github.com:osmc/osmc.git" readonly="readonly" aria-label="SSH clone URL">
    <span class="input-group-button">
      <button aria-label="Copy to clipboard" class="js-zeroclipboard btn btn-sm zeroclipboard-button tooltipped tooltipped-s" data-copied-hint="Copied!" type="button"><span class="octicon octicon-clippy"></span></button>
    </span>
  </div>
</div>

  
<div class="js-clone-url clone-url "
  data-protocol-type="subversion">
  <h3 class="text-small text-muted"><span class="text-emphasized">Subversion</span> checkout URL</h3>
  <div class="input-group js-zeroclipboard-container">
    <input type="text" class="input-mini text-small text-muted input-monospace js-url-field js-zeroclipboard-target"
           value="https://github.com/osmc/osmc" readonly="readonly" aria-label="Subversion checkout URL">
    <span class="input-group-button">
      <button aria-label="Copy to clipboard" class="js-zeroclipboard btn btn-sm zeroclipboard-button tooltipped tooltipped-s" data-copied-hint="Copied!" type="button"><span class="octicon octicon-clippy"></span></button>
    </span>
  </div>
</div>



<div class="clone-options text-small text-muted">You can clone with
  <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/users/set_protocol?protocol_selector=http&amp;protocol_type=clone" class="inline-form js-clone-selector-form is-enabled" data-form-nonce="0dd74ad73d506abb6727e8282cd469479eef0f96" data-remote="true" method="post"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /><input name="authenticity_token" type="hidden" value="PGkxZ4GbSSvBFRYBV6d/25aawqgIuphvBzHVI5Cwa8DX3IJhz7vOFpsS3F0YPa40LJ067VUfbVEkTybmK4tUzw==" /></div><button class="btn-link js-clone-selector" data-protocol="http" type="submit">HTTPS</button></form>, <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/users/set_protocol?protocol_selector=ssh&amp;protocol_type=clone" class="inline-form js-clone-selector-form is-enabled" data-form-nonce="0dd74ad73d506abb6727e8282cd469479eef0f96" data-remote="true" method="post"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /><input name="authenticity_token" type="hidden" value="f2rNwXgzyOoIQEB/Apc/ttDRCxcFao0ZoVO9jI/k5PWI/SOmP9pdGkBzVgnPLwZrFA9iD3ZDUwGV1sLfEt/54g==" /></div><button class="btn-link js-clone-selector" data-protocol="ssh" type="submit">SSH</button></form>, or <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/users/set_protocol?protocol_selector=subversion&amp;protocol_type=clone" class="inline-form js-clone-selector-form is-enabled" data-form-nonce="0dd74ad73d506abb6727e8282cd469479eef0f96" data-remote="true" method="post"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /><input name="authenticity_token" type="hidden" value="AdYxrWTw82ZkBrx2VTueAHd5M8reqs69RUTpGyMm1t1OJEw7/p+YJItIod7PfdCO9fnITi9XOkZfnnu5eW7A0w==" /></div><button class="btn-link js-clone-selector" data-protocol="subversion" type="submit">Subversion</button></form>.
  <a href="https://help.github.com/articles/which-remote-url-should-i-use" class="help tooltipped tooltipped-n" aria-label="Get help on which URL is right for you.">
    <span class="octicon octicon-question"></span>
  </a>
</div>

              <a href="/osmc/osmc/archive/master.zip"
                 class="btn btn-sm sidebar-button"
                 aria-label="Download the contents of osmc/osmc as a zip file"
                 title="Download the contents of osmc/osmc as a zip file"
                 rel="nofollow">
                <span class="octicon octicon-cloud-download"></span>
                Download ZIP
              </a>
            </div>
        </div>
        <div id="js-repo-pjax-container" class="repository-content context-loader-container" data-pjax-container>

          

<a href="/osmc/osmc/blob/bf992cef8ee301a45fc6d930e6caf23bd72c72f8/package/common.sh" class="hidden js-permalink-shortcut" data-hotkey="y">Permalink</a>

<!-- blob contrib key: blob_contributors:v21:8384d00d5ca582d22ea2544c28c6984b -->

  <div class="file-navigation js-zeroclipboard-container">
    
<div class="select-menu js-menu-container js-select-menu left">
  <button class="btn btn-sm select-menu-button js-menu-target css-truncate" data-hotkey="w"
    title="master"
    type="button" aria-label="Switch branches or tags" tabindex="0" aria-haspopup="true">
    <i>Branch:</i>
    <span class="js-select-button css-truncate-target">master</span>
  </button>

  <div class="select-menu-modal-holder js-menu-content js-navigation-container" data-pjax aria-hidden="true">

    <div class="select-menu-modal">
      <div class="select-menu-header">
        <span class="select-menu-title">Switch branches/tags</span>
        <span class="octicon octicon-x js-menu-close" role="button" aria-label="Close"></span>
      </div>

      <div class="select-menu-filters">
        <div class="select-menu-text-filter">
          <input type="text" aria-label="Filter branches/tags" id="context-commitish-filter-field" class="js-filterable-field js-navigation-enable" placeholder="Filter branches/tags">
        </div>
        <div class="select-menu-tabs">
          <ul>
            <li class="select-menu-tab">
              <a href="#" data-tab-filter="branches" data-filter-placeholder="Filter branches/tags" class="js-select-menu-tab" role="tab">Branches</a>
            </li>
            <li class="select-menu-tab">
              <a href="#" data-tab-filter="tags" data-filter-placeholder="Find a tag…" class="js-select-menu-tab" role="tab">Tags</a>
            </li>
          </ul>
        </div>
      </div>

      <div class="select-menu-list select-menu-tab-bucket js-select-menu-tab-bucket" data-tab-filter="branches" role="menu">

        <div data-filterable-for="context-commitish-filter-field" data-filterable-type="substring">


            <a class="select-menu-item js-navigation-item js-navigation-open selected"
               href="/osmc/osmc/blob/master/package/common.sh"
               data-name="master"
               data-skip-pjax="true"
               rel="nofollow">
              <span class="select-menu-item-icon octicon octicon-check"></span>
              <span class="select-menu-item-text css-truncate-target" title="master">
                master
              </span>
            </a>
        </div>

          <div class="select-menu-no-results">Nothing to show</div>
      </div>

      <div class="select-menu-list select-menu-tab-bucket js-select-menu-tab-bucket" data-tab-filter="tags">
        <div data-filterable-for="context-commitish-filter-field" data-filterable-type="substring">


        </div>

        <div class="select-menu-no-results">Nothing to show</div>
      </div>

    </div>
  </div>
</div>

    <div class="btn-group right">
      <a href="/osmc/osmc/find/master"
            class="js-show-file-finder btn btn-sm empty-icon tooltipped tooltipped-nw"
            data-pjax
            data-hotkey="t"
            aria-label="Quickly jump between files">
        <span class="octicon octicon-list-unordered"></span>
      </a>
      <button aria-label="Copy file path to clipboard" class="js-zeroclipboard btn btn-sm zeroclipboard-button tooltipped tooltipped-s" data-copied-hint="Copied!" type="button"><span class="octicon octicon-clippy"></span></button>
    </div>

    <div class="breadcrumb js-zeroclipboard-target">
      <span class="repo-root js-repo-root"><span itemscope="" itemtype="http://data-vocabulary.org/Breadcrumb"><a href="/osmc/osmc" class="" data-branch="master" data-pjax="true" itemscope="url"><span itemprop="title">osmc</span></a></span></span><span class="separator">/</span><span itemscope="" itemtype="http://data-vocabulary.org/Breadcrumb"><a href="/osmc/osmc/tree/master/package" class="" data-branch="master" data-pjax="true" itemscope="url"><span itemprop="title">package</span></a></span><span class="separator">/</span><strong class="final-path">common.sh</strong>
    </div>
  </div>

<include-fragment class="commit-tease" src="/osmc/osmc/contributors/master/package/common.sh">
  <div>
    Fetching contributors&hellip;
  </div>

  <div class="commit-tease-contributors">
    <img alt="" class="loader-loading left" height="16" src="https://assets-cdn.github.com/images/spinners/octocat-spinner-32-EAF2F5.gif" width="16" />
    <span class="loader-error">Cannot retrieve contributors at this time</span>
  </div>
</include-fragment>
<div class="file">
  <div class="file-header">
  <div class="file-actions">

    <div class="btn-group">
      <a href="/osmc/osmc/raw/master/package/common.sh" class="btn btn-sm " id="raw-url">Raw</a>
        <a href="/osmc/osmc/blame/master/package/common.sh" class="btn btn-sm js-update-url-with-hash">Blame</a>
      <a href="/osmc/osmc/commits/master/package/common.sh" class="btn btn-sm " rel="nofollow">History</a>
    </div>


        <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/osmc/osmc/edit/master/package/common.sh" class="inline-form js-update-url-with-hash" data-form-nonce="0dd74ad73d506abb6727e8282cd469479eef0f96" method="post"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /><input name="authenticity_token" type="hidden" value="CG4I1GXJnR3n/JOZN+gvbvHFDVd6OxYdd6SNKZ3+Sm7pIYjYpjznKJr/SYKamdem/pDREDtT902/OvKB2L8moA==" /></div>
          <button class="octicon-btn tooltipped tooltipped-nw" type="submit"
            aria-label="Edit the file in your fork of this project" data-hotkey="e" data-disable-with>
            <span class="octicon octicon-pencil"></span>
          </button>
</form>        <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="/osmc/osmc/delete/master/package/common.sh" class="inline-form" data-form-nonce="0dd74ad73d506abb6727e8282cd469479eef0f96" method="post"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /><input name="authenticity_token" type="hidden" value="Ns9LxvwFWQRclgqEI/Pphtf+j1CPlDvfpvwzImFivTUfJ7+/KqmGktCfE7r0ScpFeucXyCGBDd9521S2veemZA==" /></div>
          <button class="octicon-btn octicon-btn-danger tooltipped tooltipped-nw" type="submit"
            aria-label="Delete the file in your fork of this project" data-disable-with>
            <span class="octicon octicon-trashcan"></span>
          </button>
</form>  </div>

  <div class="file-info">
      <span class="file-mode" title="File mode">executable file</span>
      <span class="file-info-divider"></span>
      228 lines (212 sloc)
      <span class="file-info-divider"></span>
    7.09 KB
  </div>
</div>

  

  <div class="blob-wrapper data type-shell">
      <table class="highlight tab-size js-file-line-container" data-tab-size="8">
      <tr>
        <td id="L1" class="blob-num js-line-number" data-line-number="1"></td>
        <td id="LC1" class="blob-code blob-code-inner js-file-line"><span class="pl-c"># (c) 2014-2015 Sam Nazarko</span></td>
      </tr>
      <tr>
        <td id="L2" class="blob-num js-line-number" data-line-number="2"></td>
        <td id="LC2" class="blob-code blob-code-inner js-file-line"><span class="pl-c"># email@samnazarko.co.uk</span></td>
      </tr>
      <tr>
        <td id="L3" class="blob-num js-line-number" data-line-number="3"></td>
        <td id="LC3" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L4" class="blob-num js-line-number" data-line-number="4"></td>
        <td id="LC4" class="blob-code blob-code-inner js-file-line"><span class="pl-c">#!/bin/bash</span></td>
      </tr>
      <tr>
        <td id="L5" class="blob-num js-line-number" data-line-number="5"></td>
        <td id="LC5" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L6" class="blob-num js-line-number" data-line-number="6"></td>
        <td id="LC6" class="blob-code blob-code-inner js-file-line"><span class="pl-c1">.</span> ../../scripts/common.sh</td>
      </tr>
      <tr>
        <td id="L7" class="blob-num js-line-number" data-line-number="7"></td>
        <td id="LC7" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L8" class="blob-num js-line-number" data-line-number="8"></td>
        <td id="LC8" class="blob-code blob-code-inner js-file-line"><span class="pl-c"># Available options for building</span></td>
      </tr>
      <tr>
        <td id="L9" class="blob-num js-line-number" data-line-number="9"></td>
        <td id="LC9" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> BUILD_OPTION_LANGC=1</td>
      </tr>
      <tr>
        <td id="L10" class="blob-num js-line-number" data-line-number="10"></td>
        <td id="LC10" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> BUILD_OPTION_BUILD_FRESH=2</td>
      </tr>
      <tr>
        <td id="L11" class="blob-num js-line-number" data-line-number="11"></td>
        <td id="LC11" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> BUILD_OPTION_USE_GOLD=4</td>
      </tr>
      <tr>
        <td id="L12" class="blob-num js-line-number" data-line-number="12"></td>
        <td id="LC12" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> BUILD_OPTION_USE_O3=8</td>
      </tr>
      <tr>
        <td id="L13" class="blob-num js-line-number" data-line-number="13"></td>
        <td id="LC13" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> BUILD_OPTION_USE_NOFP=16</td>
      </tr>
      <tr>
        <td id="L14" class="blob-num js-line-number" data-line-number="14"></td>
        <td id="LC14" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> BUILD_OPTION_USE_MULTIARCH=32</td>
      </tr>
      <tr>
        <td id="L15" class="blob-num js-line-number" data-line-number="15"></td>
        <td id="LC15" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> BUILD_OPTION_USE_CCACHE=64</td>
      </tr>
      <tr>
        <td id="L16" class="blob-num js-line-number" data-line-number="16"></td>
        <td id="LC16" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> BUILD_OPTION_DEFAULTS=<span class="pl-s"><span class="pl-pds">$((</span><span class="pl-smi">$BUILD_OPTION_LANGC</span> <span class="pl-k">+</span> <span class="pl-smi">$BUILD_OPTION_USE_O3</span> <span class="pl-k">+</span> <span class="pl-smi">$BUILD_OPTION_USE_NOFP</span> <span class="pl-k">+</span> <span class="pl-smi">$BUILD_OPTION_USE_CCACHE</span><span class="pl-pds">))</span></span></td>
      </tr>
      <tr>
        <td id="L17" class="blob-num js-line-number" data-line-number="17"></td>
        <td id="LC17" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L18" class="blob-num js-line-number" data-line-number="18"></td>
        <td id="LC18" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">fix_arch_ctl()</span></td>
      </tr>
      <tr>
        <td id="L19" class="blob-num js-line-number" data-line-number="19"></td>
        <td id="LC19" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L20" class="blob-num js-line-number" data-line-number="20"></td>
        <td id="LC20" class="blob-code blob-code-inner js-file-line">	sed <span class="pl-s"><span class="pl-pds">&#39;</span>/Architecture/d<span class="pl-pds">&#39;</span></span> -i <span class="pl-smi">$1</span></td>
      </tr>
      <tr>
        <td id="L21" class="blob-num js-line-number" data-line-number="21"></td>
        <td id="LC21" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">test</span> <span class="pl-s"><span class="pl-pds">$(</span>arch<span class="pl-pds">)</span></span>x == i686x <span class="pl-k">&amp;&amp;</span> <span class="pl-c1">echo</span> <span class="pl-s"><span class="pl-pds">&quot;</span>Architecture: i386<span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;&gt;</span> <span class="pl-smi">$1</span></td>
      </tr>
      <tr>
        <td id="L22" class="blob-num js-line-number" data-line-number="22"></td>
        <td id="LC22" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">test</span> <span class="pl-s"><span class="pl-pds">$(</span>arch<span class="pl-pds">)</span></span>x == armv7lx <span class="pl-k">&amp;&amp;</span> <span class="pl-c1">echo</span> <span class="pl-s"><span class="pl-pds">&quot;</span>Architecture: armhf<span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;&gt;</span> <span class="pl-smi">$1</span></td>
      </tr>
      <tr>
        <td id="L23" class="blob-num js-line-number" data-line-number="23"></td>
        <td id="LC23" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">test</span> <span class="pl-s"><span class="pl-pds">$(</span>arch<span class="pl-pds">)</span></span>x == x86_64x <span class="pl-k">&amp;&amp;</span> <span class="pl-c1">echo</span> <span class="pl-s"><span class="pl-pds">&quot;</span>Architecture: amd64<span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;&gt;</span> <span class="pl-smi">$1</span></td>
      </tr>
      <tr>
        <td id="L24" class="blob-num js-line-number" data-line-number="24"></td>
        <td id="LC24" class="blob-code blob-code-inner js-file-line">	sed <span class="pl-s"><span class="pl-pds">&#39;</span>$!N; /^\(.*\)\n\1$/!P; D<span class="pl-pds">&#39;</span></span> -i <span class="pl-smi">$1</span></td>
      </tr>
      <tr>
        <td id="L25" class="blob-num js-line-number" data-line-number="25"></td>
        <td id="LC25" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L26" class="blob-num js-line-number" data-line-number="26"></td>
        <td id="LC26" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L27" class="blob-num js-line-number" data-line-number="27"></td>
        <td id="LC27" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">strip_files()</span></td>
      </tr>
      <tr>
        <td id="L28" class="blob-num js-line-number" data-line-number="28"></td>
        <td id="LC28" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L29" class="blob-num js-line-number" data-line-number="29"></td>
        <td id="LC29" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Stripping binaries<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L30" class="blob-num js-line-number" data-line-number="30"></td>
        <td id="LC30" class="blob-code blob-code-inner js-file-line">	strip -s <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">${1}</span>/usr/lib/*.so.*<span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;</span> /dev/null <span class="pl-k">2&gt;&amp;1</span></td>
      </tr>
      <tr>
        <td id="L31" class="blob-num js-line-number" data-line-number="31"></td>
        <td id="LC31" class="blob-code blob-code-inner js-file-line">	strip -s <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">${1}</span>/usr/lib/*.a<span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;</span> /dev/null <span class="pl-k">2&gt;&amp;1</span></td>
      </tr>
      <tr>
        <td id="L32" class="blob-num js-line-number" data-line-number="32"></td>
        <td id="LC32" class="blob-code blob-code-inner js-file-line">	strip -s <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">${1}</span>/usr/bin/*<span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;</span>/dev/null <span class="pl-k">2&gt;&amp;1</span></td>
      </tr>
      <tr>
        <td id="L33" class="blob-num js-line-number" data-line-number="33"></td>
        <td id="LC33" class="blob-code blob-code-inner js-file-line">	strip -s <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">${1}</span>/usr/sbin/*<span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;</span>/dev/null <span class="pl-k">2&gt;&amp;1</span></td>
      </tr>
      <tr>
        <td id="L34" class="blob-num js-line-number" data-line-number="34"></td>
        <td id="LC34" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L35" class="blob-num js-line-number" data-line-number="35"></td>
        <td id="LC35" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L36" class="blob-num js-line-number" data-line-number="36"></td>
        <td id="LC36" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">strip_libs()</span></td>
      </tr>
      <tr>
        <td id="L37" class="blob-num js-line-number" data-line-number="37"></td>
        <td id="LC37" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L38" class="blob-num js-line-number" data-line-number="38"></td>
        <td id="LC38" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Stripping libaries<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L39" class="blob-num js-line-number" data-line-number="39"></td>
        <td id="LC39" class="blob-code blob-code-inner js-file-line">	strip <span class="pl-s"><span class="pl-pds">&quot;</span>*.so.*<span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;</span> /dev/null <span class="pl-k">2&gt;&amp;1</span></td>
      </tr>
      <tr>
        <td id="L40" class="blob-num js-line-number" data-line-number="40"></td>
        <td id="LC40" class="blob-code blob-code-inner js-file-line">	strip <span class="pl-s"><span class="pl-pds">&quot;</span>*.a<span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;</span> /dev/null <span class="pl-k">2&gt;&amp;1</span></td>
      </tr>
      <tr>
        <td id="L41" class="blob-num js-line-number" data-line-number="41"></td>
        <td id="LC41" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L42" class="blob-num js-line-number" data-line-number="42"></td>
        <td id="LC42" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L43" class="blob-num js-line-number" data-line-number="43"></td>
        <td id="LC43" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">configure_build_env()</span></td>
      </tr>
      <tr>
        <td id="L44" class="blob-num js-line-number" data-line-number="44"></td>
        <td id="LC44" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L45" class="blob-num js-line-number" data-line-number="45"></td>
        <td id="LC45" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ -f <span class="pl-s"><span class="pl-pds">&quot;</span>/etc/resolv.conf<span class="pl-pds">&quot;</span></span> ]</td>
      </tr>
      <tr>
        <td id="L46" class="blob-num js-line-number" data-line-number="46"></td>
        <td id="LC46" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L47" class="blob-num js-line-number" data-line-number="47"></td>
        <td id="LC47" class="blob-code blob-code-inner js-file-line">		<span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Installing /etc/resolv.conf<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L48" class="blob-num js-line-number" data-line-number="48"></td>
        <td id="LC48" class="blob-code blob-code-inner js-file-line">		cp /etc/resolv.conf <span class="pl-smi">${1}</span>/etc/resolv.conf</td>
      </tr>
      <tr>
        <td id="L49" class="blob-num js-line-number" data-line-number="49"></td>
        <td id="LC49" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L50" class="blob-num js-line-number" data-line-number="50"></td>
        <td id="LC50" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ -f <span class="pl-s"><span class="pl-pds">&quot;</span>/etc/network/interfaces<span class="pl-pds">&quot;</span></span> ]</td>
      </tr>
      <tr>
        <td id="L51" class="blob-num js-line-number" data-line-number="51"></td>
        <td id="LC51" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L52" class="blob-num js-line-number" data-line-number="52"></td>
        <td id="LC52" class="blob-code blob-code-inner js-file-line">		<span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Installing /etc/network/interfaces<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L53" class="blob-num js-line-number" data-line-number="53"></td>
        <td id="LC53" class="blob-code blob-code-inner js-file-line">		cp /etc/network/interfaces <span class="pl-smi">${1}</span>/etc/network/interfaces</td>
      </tr>
      <tr>
        <td id="L54" class="blob-num js-line-number" data-line-number="54"></td>
        <td id="LC54" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L55" class="blob-num js-line-number" data-line-number="55"></td>
        <td id="LC55" class="blob-code blob-code-inner js-file-line">	HOSTNAME=<span class="pl-s"><span class="pl-pds">$(</span>cat /etc/hostname<span class="pl-pds">)</span></span></td>
      </tr>
      <tr>
        <td id="L56" class="blob-num js-line-number" data-line-number="56"></td>
        <td id="LC56" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">echo</span> <span class="pl-s"><span class="pl-pds">&quot;</span>127.0.0.1 <span class="pl-smi">$HOSTNAME</span><span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;</span> <span class="pl-smi">${1}</span>/etc/hosts</td>
      </tr>
      <tr>
        <td id="L57" class="blob-num js-line-number" data-line-number="57"></td>
        <td id="LC57" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L58" class="blob-num js-line-number" data-line-number="58"></td>
        <td id="LC58" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L59" class="blob-num js-line-number" data-line-number="59"></td>
        <td id="LC59" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">build_in_env()</span></td>
      </tr>
      <tr>
        <td id="L60" class="blob-num js-line-number" data-line-number="60"></td>
        <td id="LC60" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L61" class="blob-num js-line-number" data-line-number="61"></td>
        <td id="LC61" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ -n <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$4</span><span class="pl-pds">&quot;</span></span> ]</td>
      </tr>
      <tr>
        <td id="L62" class="blob-num js-line-number" data-line-number="62"></td>
        <td id="LC62" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L63" class="blob-num js-line-number" data-line-number="63"></td>
        <td id="LC63" class="blob-code blob-code-inner js-file-line">	    BUILD_OPTS=<span class="pl-smi">$4</span></td>
      </tr>
      <tr>
        <td id="L64" class="blob-num js-line-number" data-line-number="64"></td>
        <td id="LC64" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">else</span></td>
      </tr>
      <tr>
        <td id="L65" class="blob-num js-line-number" data-line-number="65"></td>
        <td id="LC65" class="blob-code blob-code-inner js-file-line">	    BUILD_OPTS=<span class="pl-smi">$BUILD_OPTION_DEFAULTS</span></td>
      </tr>
      <tr>
        <td id="L66" class="blob-num js-line-number" data-line-number="66"></td>
        <td id="LC66" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L67" class="blob-num js-line-number" data-line-number="67"></td>
        <td id="LC67" class="blob-code blob-code-inner js-file-line">	<span class="pl-c"># Don&#39;t get stuck in an endless loop</span></td>
      </tr>
      <tr>
        <td id="L68" class="blob-num js-line-number" data-line-number="68"></td>
        <td id="LC68" class="blob-code blob-code-inner js-file-line">	mount -t proc proc /proc <span class="pl-k">&gt;</span>/dev/null <span class="pl-k">2&gt;&amp;1</span></td>
      </tr>
      <tr>
        <td id="L69" class="blob-num js-line-number" data-line-number="69"></td>
        <td id="LC69" class="blob-code blob-code-inner js-file-line">	ischroot</td>
      </tr>
      <tr>
        <td id="L70" class="blob-num js-line-number" data-line-number="70"></td>
        <td id="LC70" class="blob-code blob-code-inner js-file-line">	chrootval=<span class="pl-smi">$?</span></td>
      </tr>
      <tr>
        <td id="L71" class="blob-num js-line-number" data-line-number="71"></td>
        <td id="LC71" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-smi">$chrootval</span> == 2 ] <span class="pl-k">||</span> [ <span class="pl-smi">$chrootval</span> == 0 ]</td>
      </tr>
      <tr>
        <td id="L72" class="blob-num js-line-number" data-line-number="72"></td>
        <td id="LC72" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L73" class="blob-num js-line-number" data-line-number="73"></td>
        <td id="LC73" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">if</span> <span class="pl-s"><span class="pl-pds">((</span>(<span class="pl-smi">$BUILD_OPTS</span> <span class="pl-k">&amp;</span> <span class="pl-smi">$BUILD_OPTION_LANGC</span>) <span class="pl-k">==</span> <span class="pl-smi">$BUILD_OPTION_LANGC</span><span class="pl-pds">))</span></span></td>
      </tr>
      <tr>
        <td id="L74" class="blob-num js-line-number" data-line-number="74"></td>
        <td id="LC74" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L75" class="blob-num js-line-number" data-line-number="75"></td>
        <td id="LC75" class="blob-code blob-code-inner js-file-line">                <span class="pl-k">export</span> LANG=C</td>
      </tr>
      <tr>
        <td id="L76" class="blob-num js-line-number" data-line-number="76"></td>
        <td id="LC76" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L77" class="blob-num js-line-number" data-line-number="77"></td>
        <td id="LC77" class="blob-code blob-code-inner js-file-line">	    <span class="pl-k">if</span> <span class="pl-s"><span class="pl-pds">((</span>(<span class="pl-smi">$BUILD_OPTS</span> <span class="pl-k">&amp;</span> <span class="pl-smi">$BUILD_OPTION_USE_GOLD</span>) <span class="pl-k">==</span> <span class="pl-smi">$BUILD_OPTION_USE_GOLD</span><span class="pl-pds">))</span></span></td>
      </tr>
      <tr>
        <td id="L78" class="blob-num js-line-number" data-line-number="78"></td>
        <td id="LC78" class="blob-code blob-code-inner js-file-line">	    <span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L79" class="blob-num js-line-number" data-line-number="79"></td>
        <td id="LC79" class="blob-code blob-code-inner js-file-line">		<span class="pl-k">export</span> LD=<span class="pl-s"><span class="pl-pds">&quot;</span>ld.gold <span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L80" class="blob-num js-line-number" data-line-number="80"></td>
        <td id="LC80" class="blob-code blob-code-inner js-file-line">	    <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L81" class="blob-num js-line-number" data-line-number="81"></td>
        <td id="LC81" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">if</span> <span class="pl-s"><span class="pl-pds">((</span>(<span class="pl-smi">$BUILD_OPTS</span> <span class="pl-k">&amp;</span> <span class="pl-smi">$BUILD_OPTION_USE_O3</span>) <span class="pl-k">==</span> <span class="pl-smi">$BUILD_OPTION_USE_O3</span><span class="pl-pds">))</span></span></td>
      </tr>
      <tr>
        <td id="L82" class="blob-num js-line-number" data-line-number="82"></td>
        <td id="LC82" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L83" class="blob-num js-line-number" data-line-number="83"></td>
        <td id="LC83" class="blob-code blob-code-inner js-file-line">                <span class="pl-k">export</span> BUILD_FLAGS+=<span class="pl-s"><span class="pl-pds">&quot;</span>-O3 <span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L84" class="blob-num js-line-number" data-line-number="84"></td>
        <td id="LC84" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L85" class="blob-num js-line-number" data-line-number="85"></td>
        <td id="LC85" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">if</span> <span class="pl-s"><span class="pl-pds">((</span>(<span class="pl-smi">$BUILD_OPTS</span> <span class="pl-k">&amp;</span> <span class="pl-smi">$BUILD_OPTION_USE_NOFP</span>) <span class="pl-k">==</span> <span class="pl-smi">$BUILD_OPTION_USE_NOFP</span><span class="pl-pds">))</span></span></td>
      </tr>
      <tr>
        <td id="L86" class="blob-num js-line-number" data-line-number="86"></td>
        <td id="LC86" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L87" class="blob-num js-line-number" data-line-number="87"></td>
        <td id="LC87" class="blob-code blob-code-inner js-file-line">                <span class="pl-k">export</span> BUILD_FLAGS+=<span class="pl-s"><span class="pl-pds">&quot;</span>-fomit-frame-pointer <span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L88" class="blob-num js-line-number" data-line-number="88"></td>
        <td id="LC88" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L89" class="blob-num js-line-number" data-line-number="89"></td>
        <td id="LC89" class="blob-code blob-code-inner js-file-line">	    CC=<span class="pl-s"><span class="pl-pds">&quot;</span>/usr/bin/gcc<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L90" class="blob-num js-line-number" data-line-number="90"></td>
        <td id="LC90" class="blob-code blob-code-inner js-file-line">	    CXX=<span class="pl-s"><span class="pl-pds">&quot;</span>/usr/bin/g++<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L91" class="blob-num js-line-number" data-line-number="91"></td>
        <td id="LC91" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">if</span> <span class="pl-s"><span class="pl-pds">((</span>(<span class="pl-smi">$BUILD_OPTS</span> <span class="pl-k">&amp;</span> <span class="pl-smi">$BUILD_OPTION_USE_CCACHE</span>) <span class="pl-k">==</span> <span class="pl-smi">$BUILD_OPTION_USE_CCACHE</span><span class="pl-pds">))</span></span></td>
      </tr>
      <tr>
        <td id="L92" class="blob-num js-line-number" data-line-number="92"></td>
        <td id="LC92" class="blob-code blob-code-inner js-file-line">	    <span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L93" class="blob-num js-line-number" data-line-number="93"></td>
        <td id="LC93" class="blob-code blob-code-inner js-file-line">		<span class="pl-k">export</span> CCACHE_DIR=<span class="pl-s"><span class="pl-pds">&quot;</span>/root/.ccache<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L94" class="blob-num js-line-number" data-line-number="94"></td>
        <td id="LC94" class="blob-code blob-code-inner js-file-line">		<span class="pl-k">export</span> CC=<span class="pl-s"><span class="pl-pds">&quot;</span>/usr/bin/ccache <span class="pl-smi">$CC</span><span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L95" class="blob-num js-line-number" data-line-number="95"></td>
        <td id="LC95" class="blob-code blob-code-inner js-file-line">		<span class="pl-k">export</span> CXX=<span class="pl-s"><span class="pl-pds">&quot;</span>/usr/bin/ccache <span class="pl-smi">$CXX</span><span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L96" class="blob-num js-line-number" data-line-number="96"></td>
        <td id="LC96" class="blob-code blob-code-inner js-file-line">	    <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L97" class="blob-num js-line-number" data-line-number="97"></td>
        <td id="LC97" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">export</span> CFLAGS+=<span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$BUILD_FLAGS</span><span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L98" class="blob-num js-line-number" data-line-number="98"></td>
        <td id="LC98" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">export</span> CXXFLAGS+=<span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$BUILD_FLAGS</span><span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L99" class="blob-num js-line-number" data-line-number="99"></td>
        <td id="LC99" class="blob-code blob-code-inner js-file-line">            <span class="pl-k">export</span> CPPFLAGS+=<span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$BUILD_FLAGS</span><span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L100" class="blob-num js-line-number" data-line-number="100"></td>
        <td id="LC100" class="blob-code blob-code-inner js-file-line">	    <span class="pl-k">return</span> 99</td>
      </tr>
      <tr>
        <td id="L101" class="blob-num js-line-number" data-line-number="101"></td>
        <td id="LC101" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L102" class="blob-num js-line-number" data-line-number="102"></td>
        <td id="LC102" class="blob-code blob-code-inner js-file-line">	umount /proc <span class="pl-k">&gt;</span>/dev/null <span class="pl-k">2&gt;&amp;1</span></td>
      </tr>
      <tr>
        <td id="L103" class="blob-num js-line-number" data-line-number="103"></td>
        <td id="LC103" class="blob-code blob-code-inner js-file-line">	update_sources</td>
      </tr>
      <tr>
        <td id="L104" class="blob-num js-line-number" data-line-number="104"></td>
        <td id="LC104" class="blob-code blob-code-inner js-file-line">	DEP=<span class="pl-smi">${1}</span></td>
      </tr>
      <tr>
        <td id="L105" class="blob-num js-line-number" data-line-number="105"></td>
        <td id="LC105" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">test</span> <span class="pl-smi">$DEP</span> == rbp2 <span class="pl-k">&amp;&amp;</span> DEP=<span class="pl-s"><span class="pl-pds">&quot;</span>armv7<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L106" class="blob-num js-line-number" data-line-number="106"></td>
        <td id="LC106" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">test</span> <span class="pl-smi">$DEP</span> == imx6 <span class="pl-k">&amp;&amp;</span> DEP=<span class="pl-s"><span class="pl-pds">&quot;</span>armv7<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L107" class="blob-num js-line-number" data-line-number="107"></td>
        <td id="LC107" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">test</span> <span class="pl-smi">$DEP</span> == vero <span class="pl-k">&amp;&amp;</span> DEP=<span class="pl-s"><span class="pl-pds">&quot;</span>armv7<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L108" class="blob-num js-line-number" data-line-number="108"></td>
        <td id="LC108" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">test</span> <span class="pl-smi">$DEP</span> == vero1 <span class="pl-k">&amp;&amp;</span> DEP=<span class="pl-s"><span class="pl-pds">&quot;</span>armv7<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L109" class="blob-num js-line-number" data-line-number="109"></td>
        <td id="LC109" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">test</span> <span class="pl-smi">$DEP</span> == rbp1 <span class="pl-k">&amp;&amp;</span> DEP=<span class="pl-s"><span class="pl-pds">&quot;</span>armv6l<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L110" class="blob-num js-line-number" data-line-number="110"></td>
        <td id="LC110" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">test</span> <span class="pl-smi">$DEP</span> == atv <span class="pl-k">&amp;&amp;</span> DEP=<span class="pl-s"><span class="pl-pds">&quot;</span>i386<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L111" class="blob-num js-line-number" data-line-number="111"></td>
        <td id="LC111" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">test</span> <span class="pl-smi">$DEP</span> == pc <span class="pl-k">&amp;&amp;</span> DEP=<span class="pl-s"><span class="pl-pds">&quot;</span>amd64<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L112" class="blob-num js-line-number" data-line-number="112"></td>
        <td id="LC112" class="blob-code blob-code-inner js-file-line">	TCDIR=<span class="pl-s"><span class="pl-pds">&quot;</span>/opt/osmc-tc/<span class="pl-smi">$DEP</span>-toolchain-osmc<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L113" class="blob-num js-line-number" data-line-number="113"></td>
        <td id="LC113" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> <span class="pl-s"><span class="pl-pds">((</span>(<span class="pl-smi">$BUILD_OPTS</span> <span class="pl-k">&amp;</span> <span class="pl-smi">$BUILD_OPTION_BUILD_FRESH</span>) <span class="pl-k">==</span> <span class="pl-smi">$BUILD_OPTION_BUILD_FRESH</span><span class="pl-pds">))</span></span></td>
      </tr>
      <tr>
        <td id="L114" class="blob-num js-line-number" data-line-number="114"></td>
        <td id="LC114" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L115" class="blob-num js-line-number" data-line-number="115"></td>
        <td id="LC115" class="blob-code blob-code-inner js-file-line">	    apt-get -y remove --purge <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$DEP</span>-toolchain-osmc<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L116" class="blob-num js-line-number" data-line-number="116"></td>
        <td id="LC116" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L117" class="blob-num js-line-number" data-line-number="117"></td>
        <td id="LC117" class="blob-code blob-code-inner js-file-line">	handle_dep <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$DEP</span>-toolchain-osmc<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L118" class="blob-num js-line-number" data-line-number="118"></td>
        <td id="LC118" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-smi">$?</span> <span class="pl-k">!</span>= 0 ]<span class="pl-k">;</span> <span class="pl-k">then</span> <span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Can&#39;t get upstream toolchain. Is apt.osmc.tv in your sources.list?<span class="pl-pds">&quot;</span></span> <span class="pl-k">&amp;&amp;</span> <span class="pl-c1">exit</span> 1<span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L119" class="blob-num js-line-number" data-line-number="119"></td>
        <td id="LC119" class="blob-code blob-code-inner js-file-line">	configure_build_env <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$TCDIR</span><span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L120" class="blob-num js-line-number" data-line-number="120"></td>
        <td id="LC120" class="blob-code blob-code-inner js-file-line">	umount <span class="pl-smi">${TCDIR}</span>/mnt <span class="pl-k">&gt;</span>/dev/null <span class="pl-k">2&gt;&amp;1</span> <span class="pl-c"># May be dirty</span></td>
      </tr>
      <tr>
        <td id="L121" class="blob-num js-line-number" data-line-number="121"></td>
        <td id="LC121" class="blob-code blob-code-inner js-file-line">	mount --bind <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span>/../../<span class="pl-pds">&quot;</span></span> <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$TCDIR</span><span class="pl-pds">&quot;</span></span>/mnt</td>
      </tr>
      <tr>
        <td id="L122" class="blob-num js-line-number" data-line-number="122"></td>
        <td id="LC122" class="blob-code blob-code-inner js-file-line">	chroot <span class="pl-smi">$TCDIR</span> /usr/bin/make <span class="pl-smi">$1</span> -C /mnt/package/<span class="pl-smi">$3</span></td>
      </tr>
      <tr>
        <td id="L123" class="blob-num js-line-number" data-line-number="123"></td>
        <td id="LC123" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">return</span>=<span class="pl-smi">$?</span></td>
      </tr>
      <tr>
        <td id="L124" class="blob-num js-line-number" data-line-number="124"></td>
        <td id="LC124" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-smi">$return</span> == 99 ]<span class="pl-k">;</span> <span class="pl-k">then</span> <span class="pl-k">return</span> 1<span class="pl-k">;</span> <span class="pl-k">else</span> <span class="pl-k">return</span> <span class="pl-smi">$return</span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L125" class="blob-num js-line-number" data-line-number="125"></td>
        <td id="LC125" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L126" class="blob-num js-line-number" data-line-number="126"></td>
        <td id="LC126" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L127" class="blob-num js-line-number" data-line-number="127"></td>
        <td id="LC127" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">teardown_env()</span></td>
      </tr>
      <tr>
        <td id="L128" class="blob-num js-line-number" data-line-number="128"></td>
        <td id="LC128" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L129" class="blob-num js-line-number" data-line-number="129"></td>
        <td id="LC129" class="blob-code blob-code-inner js-file-line">	TCDIR=<span class="pl-s"><span class="pl-pds">&quot;</span>/opt/osmc-tc/<span class="pl-smi">$1</span>-toolchain-osmc<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L130" class="blob-num js-line-number" data-line-number="130"></td>
        <td id="LC130" class="blob-code blob-code-inner js-file-line">	umount <span class="pl-smi">${TCDIR}</span>/mnt <span class="pl-k">&gt;</span>/dev/null <span class="pl-k">2&gt;&amp;1</span></td>
      </tr>
      <tr>
        <td id="L131" class="blob-num js-line-number" data-line-number="131"></td>
        <td id="LC131" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L132" class="blob-num js-line-number" data-line-number="132"></td>
        <td id="LC132" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L133" class="blob-num js-line-number" data-line-number="133"></td>
        <td id="LC133" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">handle_dep()</span></td>
      </tr>
      <tr>
        <td id="L134" class="blob-num js-line-number" data-line-number="134"></td>
        <td id="LC134" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L135" class="blob-num js-line-number" data-line-number="135"></td>
        <td id="LC135" class="blob-code blob-code-inner js-file-line">	<span class="pl-c"># Used by packages that need other packages to be built first</span></td>
      </tr>
      <tr>
        <td id="L136" class="blob-num js-line-number" data-line-number="136"></td>
        <td id="LC136" class="blob-code blob-code-inner js-file-line">	<span class="pl-c"># Check dpkg-query for the existence of the package, try install, otherwise bail.</span></td>
      </tr>
      <tr>
        <td id="L137" class="blob-num js-line-number" data-line-number="137"></td>
        <td id="LC137" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> <span class="pl-k">!</span> dpkg-query -W -f=<span class="pl-s"><span class="pl-pds">&#39;</span>${Status}<span class="pl-pds">&#39;</span></span> <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">${1}</span><span class="pl-pds">&quot;</span></span> <span class="pl-k">2&gt;</span>/dev/null <span class="pl-k">|</span> grep -q <span class="pl-s"><span class="pl-pds">&quot;</span>ok installed<span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;</span>/dev/null <span class="pl-k">2&gt;&amp;1</span></td>
      </tr>
      <tr>
        <td id="L138" class="blob-num js-line-number" data-line-number="138"></td>
        <td id="LC138" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L139" class="blob-num js-line-number" data-line-number="139"></td>
        <td id="LC139" class="blob-code blob-code-inner js-file-line">		<span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Package <span class="pl-smi">${1}</span> is not found on the system, checking APT<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L140" class="blob-num js-line-number" data-line-number="140"></td>
        <td id="LC140" class="blob-code blob-code-inner js-file-line">		<span class="pl-c"># apt-cache search always returns 0. Ugh. </span></td>
      </tr>
      <tr>
        <td id="L141" class="blob-num js-line-number" data-line-number="141"></td>
        <td id="LC141" class="blob-code blob-code-inner js-file-line">		<span class="pl-k">if</span> <span class="pl-k">!</span> apt-cache search <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">${1}</span><span class="pl-pds">&quot;</span></span> <span class="pl-k">|</span> grep -q <span class="pl-s"><span class="pl-pds">&quot;</span>^<span class="pl-smi">${1}</span> <span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L142" class="blob-num js-line-number" data-line-number="142"></td>
        <td id="LC142" class="blob-code blob-code-inner js-file-line">		<span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L143" class="blob-num js-line-number" data-line-number="143"></td>
        <td id="LC143" class="blob-code blob-code-inner js-file-line">			<span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Can&#39;t find the package in APT repo. It needs to be built first or you need to wait for upstream to add it<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L144" class="blob-num js-line-number" data-line-number="144"></td>
        <td id="LC144" class="blob-code blob-code-inner js-file-line">			<span class="pl-c1">exit</span> 1</td>
      </tr>
      <tr>
        <td id="L145" class="blob-num js-line-number" data-line-number="145"></td>
        <td id="LC145" class="blob-code blob-code-inner js-file-line">		<span class="pl-k">else</span></td>
      </tr>
      <tr>
        <td id="L146" class="blob-num js-line-number" data-line-number="146"></td>
        <td id="LC146" class="blob-code blob-code-inner js-file-line">			<span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Found in APT and will install<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L147" class="blob-num js-line-number" data-line-number="147"></td>
        <td id="LC147" class="blob-code blob-code-inner js-file-line">			<span class="pl-c"># armv7 conflicts</span></td>
      </tr>
      <tr>
        <td id="L148" class="blob-num js-line-number" data-line-number="148"></td>
        <td id="LC148" class="blob-code blob-code-inner js-file-line">			<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$1</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>vero-userland-dev-osmc<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> remove_conflicting <span class="pl-s"><span class="pl-pds">&quot;</span>rbp-userland-dev-osmc<span class="pl-pds">&quot;</span></span> <span class="pl-k">&amp;&amp;</span> remove_conflicting <span class="pl-s"><span class="pl-pds">&quot;</span>rbp-userland-osmc<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L149" class="blob-num js-line-number" data-line-number="149"></td>
        <td id="LC149" class="blob-code blob-code-inner js-file-line">			<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$1</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>rbp-userland-dev-osmc<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> remove_conflicting <span class="pl-s"><span class="pl-pds">&quot;</span>vero-userland-dev-osmc<span class="pl-pds">&quot;</span></span> <span class="pl-k">&amp;&amp;</span> remove_conflicting <span class="pl-s"><span class="pl-pds">&quot;</span>vero-userland-osmc<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L150" class="blob-num js-line-number" data-line-number="150"></td>
        <td id="LC150" class="blob-code blob-code-inner js-file-line">			<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$1</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>vero-libcec-dev-osmc<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> remove_conflicting <span class="pl-s"><span class="pl-pds">&quot;</span>rbp2-libcec-dev-osmc<span class="pl-pds">&quot;</span></span> <span class="pl-k">&amp;&amp;</span> remove_conflicting <span class="pl-s"><span class="pl-pds">&quot;</span>rbp2-libcec-osmc<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span> <span class="pl-c"># only rbp2 because armv7</span></td>
      </tr>
      <tr>
        <td id="L151" class="blob-num js-line-number" data-line-number="151"></td>
        <td id="LC151" class="blob-code blob-code-inner js-file-line">			<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$1</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>rbp2-libcec-dev-osmc<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> remove_conflicting <span class="pl-s"><span class="pl-pds">&quot;</span>vero-libcec-dev-osmc<span class="pl-pds">&quot;</span></span> <span class="pl-k">&amp;&amp;</span> remove_conflicting <span class="pl-s"><span class="pl-pds">&quot;</span>vero-libcec-osmc<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L152" class="blob-num js-line-number" data-line-number="152"></td>
        <td id="LC152" class="blob-code blob-code-inner js-file-line">			install_package <span class="pl-smi">${1}</span></td>
      </tr>
      <tr>
        <td id="L153" class="blob-num js-line-number" data-line-number="153"></td>
        <td id="LC153" class="blob-code blob-code-inner js-file-line">			<span class="pl-k">if</span> [ <span class="pl-smi">$?</span> -ne 0 ]<span class="pl-k">;</span> <span class="pl-k">then</span> <span class="pl-c1">exit</span> 1<span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L154" class="blob-num js-line-number" data-line-number="154"></td>
        <td id="LC154" class="blob-code blob-code-inner js-file-line">		<span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L155" class="blob-num js-line-number" data-line-number="155"></td>
        <td id="LC155" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">else</span></td>
      </tr>
      <tr>
        <td id="L156" class="blob-num js-line-number" data-line-number="156"></td>
        <td id="LC156" class="blob-code blob-code-inner js-file-line">		<span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Package <span class="pl-smi">${1}</span> is already installed in the environment<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L157" class="blob-num js-line-number" data-line-number="157"></td>
        <td id="LC157" class="blob-code blob-code-inner js-file-line">		<span class="pl-k">return</span> 0</td>
      </tr>
      <tr>
        <td id="L158" class="blob-num js-line-number" data-line-number="158"></td>
        <td id="LC158" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L159" class="blob-num js-line-number" data-line-number="159"></td>
        <td id="LC159" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L160" class="blob-num js-line-number" data-line-number="160"></td>
        <td id="LC160" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L161" class="blob-num js-line-number" data-line-number="161"></td>
        <td id="LC161" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">publish_applications_any()</span></td>
      </tr>
      <tr>
        <td id="L162" class="blob-num js-line-number" data-line-number="162"></td>
        <td id="LC162" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L163" class="blob-num js-line-number" data-line-number="163"></td>
        <td id="LC163" class="blob-code blob-code-inner js-file-line">	<span class="pl-c"># Used by applications that are architecture independent. These are usually metapackages with some configuration files shipped.</span></td>
      </tr>
      <tr>
        <td id="L164" class="blob-num js-line-number" data-line-number="164"></td>
        <td id="LC164" class="blob-code blob-code-inner js-file-line">	PKG_TARGETS=<span class="pl-s"><span class="pl-pds">&quot;</span>rbp1 rbp2 atv vero<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L165" class="blob-num js-line-number" data-line-number="165"></td>
        <td id="LC165" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">for</span> <span class="pl-smi">TARGET</span> <span class="pl-k">in</span> <span class="pl-smi">$PKG_TARGETS</span></td>
      </tr>
      <tr>
        <td id="L166" class="blob-num js-line-number" data-line-number="166"></td>
        <td id="LC166" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">do</span></td>
      </tr>
      <tr>
        <td id="L167" class="blob-num js-line-number" data-line-number="167"></td>
        <td id="LC167" class="blob-code blob-code-inner js-file-line">		<span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Publishing application for platform <span class="pl-smi">${TARGET}</span><span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L168" class="blob-num js-line-number" data-line-number="168"></td>
        <td id="LC168" class="blob-code blob-code-inner js-file-line">		<span class="pl-c"># No need to change id. Architecture is Any. </span></td>
      </tr>
      <tr>
        <td id="L169" class="blob-num js-line-number" data-line-number="169"></td>
        <td id="LC169" class="blob-code blob-code-inner js-file-line">		cp <span class="pl-smi">${1}</span>/app.json <span class="pl-smi">${1}</span>/<span class="pl-smi">${TARGET}</span>-<span class="pl-smi">${2}</span>.json</td>
      </tr>
      <tr>
        <td id="L170" class="blob-num js-line-number" data-line-number="170"></td>
        <td id="LC170" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">done</span></td>
      </tr>
      <tr>
        <td id="L171" class="blob-num js-line-number" data-line-number="171"></td>
        <td id="LC171" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L172" class="blob-num js-line-number" data-line-number="172"></td>
        <td id="LC172" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L173" class="blob-num js-line-number" data-line-number="173"></td>
        <td id="LC173" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">publish_applications_targeted()</span></td>
      </tr>
      <tr>
        <td id="L174" class="blob-num js-line-number" data-line-number="174"></td>
        <td id="LC174" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L175" class="blob-num js-line-number" data-line-number="175"></td>
        <td id="LC175" class="blob-code blob-code-inner js-file-line">	<span class="pl-c"># Used by applications that are architecture dependent. </span></td>
      </tr>
      <tr>
        <td id="L176" class="blob-num js-line-number" data-line-number="176"></td>
        <td id="LC176" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Publishing application for platform <span class="pl-smi">${TARGET}</span><span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L177" class="blob-num js-line-number" data-line-number="177"></td>
        <td id="LC177" class="blob-code blob-code-inner js-file-line">	<span class="pl-c"># This is a tad hacky. Architecture specific, platform independent</span></td>
      </tr>
      <tr>
        <td id="L178" class="blob-num js-line-number" data-line-number="178"></td>
        <td id="LC178" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>armv6l<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> devices=<span class="pl-s"><span class="pl-pds">&quot;</span>rbp1<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L179" class="blob-num js-line-number" data-line-number="179"></td>
        <td id="LC179" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>armv7<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> devices=<span class="pl-s"><span class="pl-pds">&quot;</span>rbp2 vero<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L180" class="blob-num js-line-number" data-line-number="180"></td>
        <td id="LC180" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>i386<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> devices=<span class="pl-s"><span class="pl-pds">&quot;</span>atv<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L181" class="blob-num js-line-number" data-line-number="181"></td>
        <td id="LC181" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>amd64<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> devices=<span class="pl-s"><span class="pl-pds">&quot;</span>pc<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L182" class="blob-num js-line-number" data-line-number="182"></td>
        <td id="LC182" class="blob-code blob-code-inner js-file-line">	<span class="pl-c"># Architecture specific, platform specific</span></td>
      </tr>
      <tr>
        <td id="L183" class="blob-num js-line-number" data-line-number="183"></td>
        <td id="LC183" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>rbp1<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> devices=<span class="pl-s"><span class="pl-pds">&quot;</span>rbp1<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L184" class="blob-num js-line-number" data-line-number="184"></td>
        <td id="LC184" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>rbp2<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> devices=<span class="pl-s"><span class="pl-pds">&quot;</span>rbp2<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L185" class="blob-num js-line-number" data-line-number="185"></td>
        <td id="LC185" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>vero<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> devices=<span class="pl-s"><span class="pl-pds">&quot;</span>vero<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L186" class="blob-num js-line-number" data-line-number="186"></td>
        <td id="LC186" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>atv1<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> devices=<span class="pl-s"><span class="pl-pds">&quot;</span>atv<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L187" class="blob-num js-line-number" data-line-number="187"></td>
        <td id="LC187" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span><span class="pl-pds">&quot;</span></span> == <span class="pl-s"><span class="pl-pds">&quot;</span>pc<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> devices=<span class="pl-s"><span class="pl-pds">&quot;</span>pc<span class="pl-pds">&quot;</span></span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L188" class="blob-num js-line-number" data-line-number="188"></td>
        <td id="LC188" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">for</span> <span class="pl-smi">device</span> <span class="pl-k">in</span> <span class="pl-smi">$devices</span></td>
      </tr>
      <tr>
        <td id="L189" class="blob-num js-line-number" data-line-number="189"></td>
        <td id="LC189" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">do</span></td>
      </tr>
      <tr>
        <td id="L190" class="blob-num js-line-number" data-line-number="190"></td>
        <td id="LC190" class="blob-code blob-code-inner js-file-line">	    cp <span class="pl-smi">${1}</span>/app.json <span class="pl-smi">${1}</span>/<span class="pl-smi">${device}</span>-<span class="pl-smi">${3}</span>.json</td>
      </tr>
      <tr>
        <td id="L191" class="blob-num js-line-number" data-line-number="191"></td>
        <td id="LC191" class="blob-code blob-code-inner js-file-line">	    sed -e s/<span class="pl-cce">\&quot;</span>id<span class="pl-cce">\&quot;</span>:<span class="pl-cce">\ \&quot;</span>/<span class="pl-cce">\&quot;</span>id<span class="pl-cce">\&quot;</span>:<span class="pl-cce">\ \&quot;</span><span class="pl-smi">${2}</span>-/ -i <span class="pl-smi">${device}</span>-<span class="pl-smi">${3}</span>.json <span class="pl-c"># set the correct package id</span></td>
      </tr>
      <tr>
        <td id="L192" class="blob-num js-line-number" data-line-number="192"></td>
        <td id="LC192" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">done</span></td>
      </tr>
      <tr>
        <td id="L193" class="blob-num js-line-number" data-line-number="193"></td>
        <td id="LC193" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L194" class="blob-num js-line-number" data-line-number="194"></td>
        <td id="LC194" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L195" class="blob-num js-line-number" data-line-number="195"></td>
        <td id="LC195" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">remove_conflicting()</span></td>
      </tr>
      <tr>
        <td id="L196" class="blob-num js-line-number" data-line-number="196"></td>
        <td id="LC196" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L197" class="blob-num js-line-number" data-line-number="197"></td>
        <td id="LC197" class="blob-code blob-code-inner js-file-line">	<span class="pl-c"># This is not ideal...</span></td>
      </tr>
      <tr>
        <td id="L198" class="blob-num js-line-number" data-line-number="198"></td>
        <td id="LC198" class="blob-code blob-code-inner js-file-line">	ischroot</td>
      </tr>
      <tr>
        <td id="L199" class="blob-num js-line-number" data-line-number="199"></td>
        <td id="LC199" class="blob-code blob-code-inner js-file-line">	chrootval=<span class="pl-smi">$?</span></td>
      </tr>
      <tr>
        <td id="L200" class="blob-num js-line-number" data-line-number="200"></td>
        <td id="LC200" class="blob-code blob-code-inner js-file-line">	<span class="pl-c"># guard</span></td>
      </tr>
      <tr>
        <td id="L201" class="blob-num js-line-number" data-line-number="201"></td>
        <td id="LC201" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-smi">$chrootval</span> == 2 ] <span class="pl-k">||</span> [ <span class="pl-smi">$chrootval</span> == 0 ]</td>
      </tr>
      <tr>
        <td id="L202" class="blob-num js-line-number" data-line-number="202"></td>
        <td id="LC202" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">then</span></td>
      </tr>
      <tr>
        <td id="L203" class="blob-num js-line-number" data-line-number="203"></td>
        <td id="LC203" class="blob-code blob-code-inner js-file-line">		dpkg --list <span class="pl-k">|</span> grep -q <span class="pl-smi">$1</span></td>
      </tr>
      <tr>
        <td id="L204" class="blob-num js-line-number" data-line-number="204"></td>
        <td id="LC204" class="blob-code blob-code-inner js-file-line">		<span class="pl-k">if</span> [ <span class="pl-smi">$?</span> == 0 ]<span class="pl-k">;</span> <span class="pl-k">then</span> <span class="pl-c1">echo</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span>Removing conflicting package <span class="pl-smi">$1</span><span class="pl-pds">&quot;</span></span> <span class="pl-k">&amp;&amp;</span> apt-get remove -y --purge <span class="pl-smi">$1</span><span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L205" class="blob-num js-line-number" data-line-number="205"></td>
        <td id="LC205" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L206" class="blob-num js-line-number" data-line-number="206"></td>
        <td id="LC206" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L207" class="blob-num js-line-number" data-line-number="207"></td>
        <td id="LC207" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L208" class="blob-num js-line-number" data-line-number="208"></td>
        <td id="LC208" class="blob-code blob-code-inner js-file-line"><span class="pl-k">function</span> <span class="pl-en">dpkg_build()</span></td>
      </tr>
      <tr>
        <td id="L209" class="blob-num js-line-number" data-line-number="209"></td>
        <td id="LC209" class="blob-code blob-code-inner js-file-line">{</td>
      </tr>
      <tr>
        <td id="L210" class="blob-num js-line-number" data-line-number="210"></td>
        <td id="LC210" class="blob-code blob-code-inner js-file-line">	<span class="pl-c"># Calculate package size and update control file before packaging.</span></td>
      </tr>
      <tr>
        <td id="L211" class="blob-num js-line-number" data-line-number="211"></td>
        <td id="LC211" class="blob-code blob-code-inner js-file-line">	<span class="pl-k">if</span> [ <span class="pl-k">!</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$1</span><span class="pl-pds">&quot;</span></span> -o <span class="pl-k">!</span> -e <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$1</span>/DEBIAN/control<span class="pl-pds">&quot;</span></span> ]<span class="pl-k">;</span> <span class="pl-k">then</span> <span class="pl-c1">exit</span> 1<span class="pl-k">;</span> <span class="pl-k">fi</span></td>
      </tr>
      <tr>
        <td id="L212" class="blob-num js-line-number" data-line-number="212"></td>
        <td id="LC212" class="blob-code blob-code-inner js-file-line">	sed <span class="pl-s"><span class="pl-pds">&#39;</span>/^Installed-Size/d<span class="pl-pds">&#39;</span></span> -i <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$1</span>/DEBIAN/control<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L213" class="blob-num js-line-number" data-line-number="213"></td>
        <td id="LC213" class="blob-code blob-code-inner js-file-line">	size=<span class="pl-s"><span class="pl-pds">$(</span>du -s --apparent-size <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$1</span><span class="pl-pds">&quot;</span></span> <span class="pl-k">|</span> awk <span class="pl-s"><span class="pl-pds">&#39;</span>{print $1}<span class="pl-pds">&#39;</span></span><span class="pl-pds">)</span></span></td>
      </tr>
      <tr>
        <td id="L214" class="blob-num js-line-number" data-line-number="214"></td>
        <td id="LC214" class="blob-code blob-code-inner js-file-line">	<span class="pl-c1">echo</span> <span class="pl-s"><span class="pl-pds">&quot;</span>Installed-Size: <span class="pl-smi">$size</span><span class="pl-pds">&quot;</span></span> <span class="pl-k">&gt;&gt;</span> <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$1</span>/DEBIAN/control<span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L215" class="blob-num js-line-number" data-line-number="215"></td>
        <td id="LC215" class="blob-code blob-code-inner js-file-line">	dpkg -b <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$1</span><span class="pl-pds">&quot;</span></span> <span class="pl-s"><span class="pl-pds">&quot;</span><span class="pl-smi">$2</span><span class="pl-pds">&quot;</span></span></td>
      </tr>
      <tr>
        <td id="L216" class="blob-num js-line-number" data-line-number="216"></td>
        <td id="LC216" class="blob-code blob-code-inner js-file-line">}</td>
      </tr>
      <tr>
        <td id="L217" class="blob-num js-line-number" data-line-number="217"></td>
        <td id="LC217" class="blob-code blob-code-inner js-file-line">
</td>
      </tr>
      <tr>
        <td id="L218" class="blob-num js-line-number" data-line-number="218"></td>
        <td id="LC218" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> -f fix_arch_ctl</td>
      </tr>
      <tr>
        <td id="L219" class="blob-num js-line-number" data-line-number="219"></td>
        <td id="LC219" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> -f strip_files</td>
      </tr>
      <tr>
        <td id="L220" class="blob-num js-line-number" data-line-number="220"></td>
        <td id="LC220" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> -f strip_libs</td>
      </tr>
      <tr>
        <td id="L221" class="blob-num js-line-number" data-line-number="221"></td>
        <td id="LC221" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> -f build_in_env</td>
      </tr>
      <tr>
        <td id="L222" class="blob-num js-line-number" data-line-number="222"></td>
        <td id="LC222" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> -f teardown_env</td>
      </tr>
      <tr>
        <td id="L223" class="blob-num js-line-number" data-line-number="223"></td>
        <td id="LC223" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> -f handle_dep</td>
      </tr>
      <tr>
        <td id="L224" class="blob-num js-line-number" data-line-number="224"></td>
        <td id="LC224" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> -f publish_applications_any</td>
      </tr>
      <tr>
        <td id="L225" class="blob-num js-line-number" data-line-number="225"></td>
        <td id="LC225" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> -f publish_applications_targeted</td>
      </tr>
      <tr>
        <td id="L226" class="blob-num js-line-number" data-line-number="226"></td>
        <td id="LC226" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> -f remove_conflicting</td>
      </tr>
      <tr>
        <td id="L227" class="blob-num js-line-number" data-line-number="227"></td>
        <td id="LC227" class="blob-code blob-code-inner js-file-line"><span class="pl-k">export</span> -f dpkg_build</td>
      </tr>
</table>

  </div>

</div>

<a href="#jump-to-line" rel="facebox[.linejump]" data-hotkey="l" style="display:none">Jump to Line</a>
<div id="jump-to-line" style="display:none">
  <!-- </textarea> --><!-- '"` --><form accept-charset="UTF-8" action="" class="js-jump-to-line-form" method="get"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /></div>
    <input class="linejump-input js-jump-to-line-field" type="text" placeholder="Jump to line&hellip;" aria-label="Jump to line" autofocus>
    <button type="submit" class="btn">Go</button>
</form></div>

        </div>
      </div>
      <div class="modal-backdrop"></div>
    </div>
  </div>


    </div>

      <div class="container">
  <div class="site-footer" role="contentinfo">
    <ul class="site-footer-links right">
        <li><a href="https://status.github.com/" data-ga-click="Footer, go to status, text:status">Status</a></li>
      <li><a href="https://developer.github.com" data-ga-click="Footer, go to api, text:api">API</a></li>
      <li><a href="https://training.github.com" data-ga-click="Footer, go to training, text:training">Training</a></li>
      <li><a href="https://shop.github.com" data-ga-click="Footer, go to shop, text:shop">Shop</a></li>
        <li><a href="https://github.com/blog" data-ga-click="Footer, go to blog, text:blog">Blog</a></li>
        <li><a href="https://github.com/about" data-ga-click="Footer, go to about, text:about">About</a></li>
        <li><a href="https://github.com/pricing" data-ga-click="Footer, go to pricing, text:pricing">Pricing</a></li>

    </ul>

    <a href="https://github.com" aria-label="Homepage">
      <span class="mega-octicon octicon-mark-github" title="GitHub"></span>
</a>
    <ul class="site-footer-links">
      <li>&copy; 2015 <span title="0.08529s from github-fe122-cp1-prd.iad.github.net">GitHub</span>, Inc.</li>
        <li><a href="https://github.com/site/terms" data-ga-click="Footer, go to terms, text:terms">Terms</a></li>
        <li><a href="https://github.com/site/privacy" data-ga-click="Footer, go to privacy, text:privacy">Privacy</a></li>
        <li><a href="https://github.com/security" data-ga-click="Footer, go to security, text:security">Security</a></li>
        <li><a href="https://github.com/contact" data-ga-click="Footer, go to contact, text:contact">Contact</a></li>
        <li><a href="https://help.github.com" data-ga-click="Footer, go to help, text:help">Help</a></li>
    </ul>
  </div>
</div>



    
    
    

    <div id="ajax-error-message" class="flash flash-error">
      <span class="octicon octicon-alert"></span>
      <button type="button" class="flash-close js-flash-close js-ajax-error-dismiss" aria-label="Dismiss error">
        <span class="octicon octicon-x"></span>
      </button>
      Something went wrong with that request. Please try again.
    </div>


      <script crossorigin="anonymous" src="https://assets-cdn.github.com/assets/frameworks-080f1c155a28f5a4315d4a6862aeafb7e27bca0a74db6f7ae9e0048e321369d1.js"></script>
      <script async="async" crossorigin="anonymous" src="https://assets-cdn.github.com/assets/github-d93d228828812b57da6aabbd4454c639406e0d694d2b8f6584a190548afdf5d5.js"></script>
      
      
    <div class="js-stale-session-flash stale-session-flash flash flash-warn flash-banner hidden">
      <span class="octicon octicon-alert"></span>
      <span class="signed-in-tab-flash">You signed in with another tab or window. <a href="">Reload</a> to refresh your session.</span>
      <span class="signed-out-tab-flash">You signed out in another tab or window. <a href="">Reload</a> to refresh your session.</span>
    </div>
  </body>
</html>

