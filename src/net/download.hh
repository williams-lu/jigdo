/* $Id: download.hh,v 1.3 2003-08-05 15:04:59 atterer Exp $ -*- C++ -*-
  __   _
  |_) /|  Copyright (C) 2001-2003  |  richard@
  | \/�|  Richard Atterer          |  atterer.net
  � '` �
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2. See
  the file COPYING for details.

  Download data from URL, write to output function, report on progress

  This is an abstraction of the HTTP/FTP download facility. The rest of the
  program only calls libwww indirectly through this code.

*/

#ifndef DOWNLOAD_HH
#define DOWNLOAD_HH

#include <config.h>

#include <glib.h>
#include <string>

#include <libwww.hh>

#ifdef ERROR
#  undef ERROR /* Windows... */
#endif
//______________________________________________________________________

/** Class containing the state of a download. libwww must be properly
    initialised before usage (this happens in glibwww-init.cc).

    The purpose of this class is a minimal API for downloading HTTP and FTP
    URLs. A few useful extra features are added by Job::SingleURL.

    The Output object is e.g. a GtkSingleUrl for single-file downloads */
class Download {
public:
  /// Class for outputting progress reports and data
  class Output;

  // Initialize libwww - call this before starting any downloads
  static void init();

  Download(const string& uri, Output* o /*= 0*/);
  ~Download();

  /** Start downloading. Returns (almost) immediately and runs the download
      in the background via the glib/libwww event loop. If resumeOffset > 0,
      continue downloading the data from a given byte position. If a resume
      is not possible (e.g. server does not support HTTP ranges), there's an
      error. */
  void run(uint64 resumeOffset, bool pragmaNoCache);

  inline const string& uri() const;

  /** Pause the request. Actually, this only sets a flag, which causes the
      request to be paused at the next convenient opportunity. The socket of
      the Download is no longer passed to select(). */
  inline void pause();
  /** Continue with the download */
  void cont();
  /** Is the download paused? (i.e. really paused now, not just pause()
      called.) */
  inline bool paused() const;
  /** Is the download paused, or is it not yet paused, but will be soon? */
  inline bool pausedSoon() const;
  /** Did the download fail with an error? */
  inline bool failed() const;
  /** Is download finished? (Also returns true if FTP/HTTP1.0 connection
      dropped) */
  inline bool succeeded() const;
  /** The connection was interrupted during the download, or it timed out. */
  inline bool interrupted() const;

  /** Forcibly stop this download. Careful, in case we are pipelining it is
      unavoidable that other (pending and not pending) downloads in the same
      pipeline also fail; they'll have to be resumed.
      stop() must not be called when libwww is delivering data, i.e. you must
      not call it from your download_data() method. */
  void stop();

  /** Value passed to resume(), or 0 if downloading from start. */
  inline uint64 resumeOffset() const;

  /** Return the Output object */
  inline Output* output() const;
  /** Set the Output object */
  inline void setOutput(Output* o);

private:
  enum State {
    CREATED, // but not run() yet
    RUNNING, // downloading
    PAUSE_SCHEDULED, // will switch to pause next time data arrives
    PAUSED, // socket no longer being polled, we'll get no more data
    INTERRUPTED, // like ERROR, but will try resuming the download
    ERROR, SUCCEEDED
  };

  // Function called by libwww for progress reports
  static BOOL alertCallback(HTRequest* request, HTAlertOpcode op, int msgnum,
                            const char* dfault, void* input,
                            HTAlertPar* reply);
  // Function called by libwww on error
  static int afterFilter(HTRequest* request, HTResponse* response,
                         void* param, int status);
  // Function called by libwww when a request is (re)started
  static int activateRequestCallback(HTRequest* request);

  // libwww HTStream interface
  static int flush(HTStream* me);
  static int free(HTStream* me);
  static int abort(HTStream* me, HTList* e);
  static int putChar(HTStream* me, char c);
  static int putString(HTStream* me, const char* s);
  static int write(HTStream* me, const char* s, int l);
  // Function called by libwww, just before any alertCallback
  static int requestCallback(HTRequest* request, void* downloadObj);
  // Unregister request from glibwww event loop
  void pauseNow();
  // Call output->error() with appropriate string taken from request object
  void generateError(State newState = ERROR);
  // Set resumeChecked=true or create an error. Returns true if error.
  bool resumeCheck();

  /* In order for this object to be a libwww stream, we need to supply a
     "vtbl pointer" for HTStream AS THE FIRST DATA MEMBER. */
  const HTStreamClass* vptr;
  string uriVal;
  uint64 resumeOffsetVal;
  bool resumeChecked; // false => still need to check for right Content-Range
  uint64 currentSize;
  Output* outputVal; // Usually points to a Job::SingleUrl
  HTRequest* request;
  State state;

//   unsigned char activated; // Count of calls to activateRequestCallback
//   static gboolean destroyRequestCallback(gpointer data);
//   int destroyRequestId; // glib event ID for destroyRequestCallback

  static string userAgent;
# if DEBUG
  bool insideNewData;
# endif
};
//______________________________________________________________________

/** A derived class of Output must be supplied by the user of Download, to
    take care of the downloaded data and to output progress reports. */
class Download::Output {
public:

  /** The Download object does *not* delete its Output. */
  virtual ~Output() { }

  /* Called by the Download when it is deleted. If the Output object
      considers itself owned by its Download, it can delete itself. */
  //virtual void download_deleted() = 0;

  /** Called as soon as the size of the downloaded data is known. May not be
      called at all if the size is unknown. May be called before the first
      download_data() or sometime afterwards.
      Problem with libwww: Returns size as long int - 2 GB size limit! */
  virtual void download_dataSize(uint64 n) = 0;

  /** Called during download whenever data arrives, with the data that just
      arrived. You can write the data to a file, copy it away etc.
      currentSize is the offset into the downloaded data (including the
      "size" new bytes) - useful for "x% done" messages. */
  virtual void download_data(const byte* data, unsigned size,
                             uint64 currentSize) = 0;

  /** Called when download is complete. If this is called, you may want to
      check whether dataSize() was ever called, and if it was, check whether
      currentSize == dataSize. If this is not the case (i.e. less data was
      received than the amount promised by the server), consider resume()ing
      the download. */
  virtual void download_succeeded() = 0;

  /** Short progress message ("Timeout", "404 not found") which means that
      the download has failed. From now on, Download::failed() will be true.
      Message is always valid UTF-8 */
  virtual void download_failed(string* message) = 0;

  /** Short progress message ("Looking up host", "Connecting" etc). As a
      special case, an empty string indicates that the download proper has
      begun. Message is always valid UTF-8 */
  virtual void download_message(string* message) = 0;

};
//______________________________________________________________________

const string& Download::uri() const { return uriVal; }

Download::Output* Download::output() const { return outputVal; }
void Download::setOutput(Download::Output* o) { outputVal = o; }

void Download::pause() {
  if (state == RUNNING) state = PAUSE_SCHEDULED;
}
bool Download::paused() const { return state == PAUSED; }
bool Download::pausedSoon() const {
  return state == PAUSED || state == PAUSE_SCHEDULED; }
bool Download::failed() const { return state == ERROR; }
bool Download::succeeded() const { return state == SUCCEEDED; }
bool Download::interrupted() const { return state == INTERRUPTED; }

uint64 Download::resumeOffset() const { return resumeOffsetVal; }

#endif
