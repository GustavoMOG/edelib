/*
 * $Id: DesktopFile.h 3576 2014-06-20 10:04:55Z karijes $
 *
 * .desktop file reader and writer
 * Copyright (c) 2005-2014 edelib authors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __EDELIB_DESKTOPFILE_H__
#define __EDELIB_DESKTOPFILE_H__

#include "Config.h"
#include "String.h"
#include "List.h"

EDELIB_NS_BEGIN

/**
 * \enum DesktopFileErrors
 * \brief Error codes from DesktopFile class
 */
enum DesktopFileErrors {
	DESK_FILE_SUCCESS = 0,   ///< successful operation
	DESK_FILE_EMPTY,         ///< file not loaded
	DESK_FILE_ERR_FILE,      ///< trouble accessing config file or directory
	DESK_FILE_ERR_BAD        ///< malformed file, or not .desktop file
};

/**
 * \enum DesktopFileType
 * \brief Reported file type from DesktopFile::type()
 */
enum DesktopFileType {
	DESK_FILE_TYPE_UNKNOWN = 0,   ///< Unknown type (Type key)
	DESK_FILE_TYPE_APPLICATION,   ///< Application type
	DESK_FILE_TYPE_LINK,          ///< Link type
	DESK_FILE_TYPE_DIRECTORY      ///< Directory type
};

/**
 * \class DesktopFile
 * \brief .desktop file reader and writer
 *
 * Desktop file (or desktop entries) are config files ending with <em>.desktop</em>
 * extension. This is text file, readable with Config class. These files are
 * (ab)used to describe desktop icons, menu entries and etc.
 *
 * Current implementation conforms to http://www.freedesktop.org specification version 1.1.
 *
 * In case you want to create .desktop file from scratch, <em>make sure</em> you
 * call create_new() before you fill the data, so DesktopFile can setup mandatory data prior file saving, like:
 * \code
 *   DesktopFile f;
 *   f.create_new(DESK_FILE_TYPE_APPLICATION);
 *   f.set_name("MyApp");
 *   f.save("myapp.desktop");
 * \endcode
 *
 * create_new() will also clear all current hold conten
 */

class EDELIB_API DesktopFile : public Config {
private:
	unsigned int errcode;
	DesktopFileType dtype;

public:
	/**
	 * Initialize internal data
	 */
	DesktopFile();

	/**
	 * Empty destructor
	 */
	~DesktopFile();

	/**
	 * Load file checking if is .desktop file
	 * \return true if file loaded or false if error occured
	 * \param fname is file to load
	 */
	bool load(const char* fname);	

	/**
	 * Save file
	 * \return true if was succesfull or false if not
	 * \param fname is file to be saved
	 */
	bool save(const char* fname);	

	/**
	 * Create new .desktop context. To create new .desktop file,
	 * you must call this function first and then fill it
	 */
	void create_new(DesktopFileType t);

	/**
	 * Used to check if current loaded file is .desktop file. Can be used like:
	 * \code
	 *   DesktopFile desk;
	 *   desk.load("foo.desktop");
	 *   if(!desk)
	 *      // save the world
	 * \endcode
	 */
	operator bool(void) const   { return ((errcode == DESK_FILE_SUCCESS) ? 1 : 0); }

	/**
	 * Return type of desktop file. It will be one of DesktopFileType
	 */
	DesktopFileType type(void);

	/**
	 * Get value from <em>Name</em> key. Returning value will match current locale.
	 * \return true if key exists and was readed, or false
	 * \param val is destination
	 * \param len size of destination
	 */
	bool name(char* val, int len);

	/**
	 * Get value from <em>GenericName</em> key. Returning value will match current locale.
	 * \return true if key exists and was readed, or false
	 * \param val is destination
	 * \param len size of destination
	 */
	bool generic_name(char* val, int len);

	/**
	 * Get value from <em>Comment</em> key. Returning value will match current locale.
	 * \return true if key exists and was readed, or false
	 * \param val is destination
	 * \param len size of destination
	 */
	bool comment(char* val, int len);

	/**
	 * Get value from <em>Icon</em> key. Returned value can be absolute path (with given icon
	 * extension) or just icon name (without extension, which is often done). In latter case
	 * final application should figure out will use IconTheme or load icon directly.
	 */
	bool icon(char* val, int len);

	/**
	 * Get value from <em>Exec</em> key. The value should be an executable program optionally followed 
	 * by one or more arguments. If executable program wasn't specified with its full path, the executable is
	 * looked up in $PATH environment variable. If not found, this function will return false.
	 *
	 * \todo special variables and symbol escaping are not implemented yet
	 */
	bool exec(char* val, int len);

	/**
	 * Get value from <em>TryExec</em> key. <em>TryExec</em> key contains program executable name and this
	 * function will try to determine if the program is actually installed. This will be done by looking
	 * either if full path was given or in $PATH environment variable.
	 *
	 * \return false if <em>TryExec</em> is not present in the file
	 * \param program_found will be true if program found; otherwise will be false
	 */
	bool try_exec(bool& program_found);

	/**
	 * Get value from <em>Path</em> key. If value is not empty and type is DESK_FILE_TYPE_APPLICATION
	 * it will fill destination and return true. Otherwise, false.
	 */
	bool path(char* val, int len);

	/**
	 * Get value from <em>URL</em> key. If value is not empty and type is DESK_FILE_TYPE_LINK
	 * it will fill destination and return true. Otherwise, false.
	 */
	bool url(char* val, int len);

	/**
	 * Get value from <em>MimeType</em> key.
	 */
	bool mime_type(char* val, int len);

	/**
	 * Get value from <em>NoDisplay</em> key.
	 */
	bool no_display(void);

	/**
	 * Get value from <em>Hidden</em> key.
	 */
	bool hidden(void);

	/**
	 * Get value from <em>Terminal</em> key.
	 */
	bool terminal(void);

	/**
	 * Get value from <em>StartupNotify</em> key.
	 */
	bool startup_notify(void);

	/**
	 * Get full value from <em>OnlyShowIn</em>. This value will be in form: <em>foo;baz;</em>.
	 */
	bool only_show_in(char* val, int len);

	/**
	 * Get list of values from <em>OnlyShowIn</em> key.
	 * If key wasn't found, list will not be altered.
	 */
	bool only_show_in(list<String>& lst);

	/**
	 * Get full value from <em>NotShowIn</em>. This value
	 * will be in form: <em>foo;baz;</em>.
	 */
	bool not_show_in(char* val, int len);

	/**
	 * Get list of values from <em>NotShowIn</em> key.
	 * If key wasn't found, list will not be altered.
	 */
	bool not_show_in(list<String>& lst);

	/**
	 * Set type of desktop file
	 */
	void set_type(DesktopFileType t);

	/**
	 * Set Name key
	 */
	void set_name(const char* val);

	/**
	 * Set GenericName key
	 */
	void set_generic_name(const char* val);

	/**
	 * Set Comment key
	 */
	void set_comment(const char* val);

	/**
	 * Set Icon key
	 */
	void set_icon(const char* val);

	/**
	 * Set Exec key
	 */
	void set_exec(const char* val);

	/**
	 * Set TryExec key
	 */
	void set_try_exec(const char* val);

	/**
	 * Set Path key
	 */
	void set_path(const char* val);

	/**
	 * Set URL key
	 */
	void set_url(const char* val);

	/**
	 * Set MimeType key
	 */
	void set_mime_type(const char* val);

	/**
	 * Set NoDisplay key
	 */
	void set_no_display(bool val);

	/**
	 * Set Hidden key
	 */
	void set_hidden(bool val);

	/**
	 * Set Terminal key
	 */
	void set_terminal(bool val);

	/**
	 * Set StartupNotify key
	 */
	void set_startup_notify(bool val);

	/**
	 * Set OnlyShowIn key. Note that only OnlyShowIn or NotShowIn keys
	 * can exists. If one of them is already set, this function will do nothing.
	 */
	void set_only_show_in(const list<String>& lst);

	/**
	 * Set NotShowIn key. Note that only OnlyShowIn or NotShowIn keys
	 * can exists. If one of them is already set, this function will do nothing.
	 */
	void set_not_show_in(const list<String>& lst);
};

EDELIB_NS_END
#endif
