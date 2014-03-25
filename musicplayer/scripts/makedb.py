#!/usr/bin/python
import sqlite3
import sys
import os
import os.path

basePath = '';

def scanDir(path, depth, add_file) :
	global dbc
	global db
	try:
		content = [os.path.join(path, x) for x in os.listdir(path)]
	except OSError:
		print >>sys.stderr, "# problem with {0}".format(path)
		return

	dirs = sorted([x for x in content if os.path.isdir(x)])
	files = sorted([x for x in content if os.path.isfile(x)])

	for d in dirs:
		if os.path.islink(d):
			continue
		scanDir(d, depth + 1, add_file)

	print "Adding", path

	for f in files :
		add_file(f)

def add_exotic(f) :
	name = f.lower()
	title = "Unnamed";
	composer = "<?>";
	year = "";
	if name.endswith('.txt') and not name.endswith('composer.txt') :
		lines = open(f, 'rb').readlines()
		for i in range(0, len(lines)-1, 2) :
			l = lines[i].strip()
			a = lines[i+1].strip()
			#print l,a
			if l == "composer 1" :
				composer = a
			elif l == "source title" :
				title = a
			elif l == "year" :
				year = a
			elif l == "group" :
				group = a

		metaData = ''
		if year :
			metaData = ("YEAR=" + year)
		f = f.replace(basePath, '')
		f = f.replace(".txt", ".lha")
		print "%s - %s [%s]" % (composer, title, f)
		dbc.execute('insert into songs values (null, ?,?,?,?)', (str(f), title, composer, metaData))




def add_hvsc(f) :
	if f.lower().endswith('.sid') :
		data = open(f, 'rb').read()
		title = data[22:22+32].strip('\x00') #  "ISO-8859-1"
		composer = data[54:54+32].strip('\x00')
		copyright = data[86:86+32].strip('\x00')	

		titleu = title.replace(' ', '_')
		composeru = composer.replace(' ', '_')
		#f = f.replace('C64Music/', '')
		f = f.replace(basePath, '')
		#f = f.replace(composeru, '%c')
		#f = f.replace(titleu, '%t')
		#f = f.replace('MUSICIANS/', '%M')
		#f = f.replace('DEMOS/', '%D')
		#f = f.replace('GAMES/', '%G')
		#print f, title, composer, copyright
		dbc.execute('insert into songs values (null, ?,?,?,?)', (str(f), title, composer, 'COPYRIGHT='+copyright))



def main(argv) :
	make_hvsc(argv[0])
	#make_modland(argv[0])
	#make_exotic(argv[0])

def make_exotic(path) :
	global dbc
	global db
	global basePath
	db = sqlite3.connect('unexotica.db')
	db.text_factory = str

	basePath = path
	if basePath[-1] != '/' :
		basePath += '/'
	dbc = db.cursor()
	try :
		dbc.execute('create table songs (_id INTEGER PRIMARY KEY, path TEXT, title TEXT, composer TEXT, metadata TEXT)')
		db.commit()
	except :
		print "DB FAILED"
		return 0
	scanDir(path, 0, add_exotic)
	db.commit()



def make_hvsc(name):
	global dbc
	global db

	db = sqlite3.connect('hvsc.db')
	db.text_factory = str

	dbc = db.cursor()
	try :
		dbc.execute('create table songs (_id INTEGER PRIMARY KEY, path TEXT, title TEXT, composer TEXT, metadata TEXT)')
#		dbc.execute('create index nameidx on songs (name)')
#		dbc.execute('create table songs (_id INTEGER PRIMARY KEY, name TEXT, author INTEGER, game INTEGER, type INTEGER)')
#		dbc.execute('create table types (_id INTEGER PRIMARY KEY, tname TEXT)')
#		dbc.execute('create table authors (_id INTEGER PRIMARY KEY, aname TEXT)')
#		dbc.execute('create table games (_id INTEGER PRIMARY KEY, gname TEXT)')
		db.commit()
	except :
		print "DB FAILED"
		return 0
	

	scanDir(name, 0, add_hvsc)
	db.commit()

	return 0;

def make_modland(name):
	types = {}
	authors = {}
	games = {}

	db = sqlite3.connect('modland.db')
	db.text_factory = str

	dbc = db.cursor()

	try :
		dbc.execute('create table songs2 (_id INTEGER PRIMARY KEY, path TEXT, title TEXT, composer TEXT, metadata TEXT)')
#		dbc.execute('create index nameidx on songs (name)')
#		dbc.execute('create table songs (_id INTEGER PRIMARY KEY, name TEXT, author INTEGER, game INTEGER, type INTEGER)')
#		dbc.execute('create table types (_id INTEGER PRIMARY KEY, tname TEXT)')
#		dbc.execute('create table authors (_id INTEGER PRIMARY KEY, aname TEXT)')
#		dbc.execute('create table games (_id INTEGER PRIMARY KEY, gname TEXT)')
		db.commit()
	except :
		print "DB FAILED"
		return 0
	
	count = 0
	for l in open(name) :
		#count+=1
		#if count > 100 :
		#	break;
		data = l.split('\t')[1].strip()
		parts = data.split('\\')
		if parts[0] == 'Ad Lib' or parts[0] == 'Video Game Music':
			parts = [parts[0] + '/' + parts[1]] + parts[2:]
		if parts[0] == 'YM' and parts[1] == 'Synth Pack':
			parts = [parts[0] + '/' + parts[1]] + parts[2:]

		if parts[2].startswith('coop-') :
			parts = [parts[0]] + [parts[1] + '+' + parts[2][5:]] + parts[3:]
			
		if len(parts) == 5 and (parts[3].startswith('instr') or parts[3].startswith('songs')) :
			parts = parts[:2] + [parts[3] + '/' + parts[4]]
			
		if len(parts) > 4 :
			parts = parts[:2] + ['/'.join(parts[3:])]
		

		type = parts[0]
		author = parts[1]
		#game = 'NONE'

		if author == "- unknown" :
			author = "<?>" ;

		ext = "";

		if len(parts) == 3 :
			#name = parts[2]
			x = os.path.splitext(parts[2])
			name = x[0]
			ext = x[1]
		elif len(parts) == 4 :			
			#game = parts[2]
			x = os.path.splitext(parts[3])
			ext = x[1]
			name = parts[2] + " (" + x[0] + ")";

		#elif len(parts) == 5 :
		#	author = author + ' ' + parts[2]
		#	game = parts[3]
		#	name = parts[4]
		else :
			print "Strange line ", parts
			raise Exception('Unknown format')


		if ext == ".smpl" or ext == ".ins" :
			print "Skipping instrument file"
			continue
		
		try:
#			if types.has_key(type) :
#				tid = types[type]
#			else :
#				dbc.execute('insert into types values (null, ?)', (type,))
#				tid = types[type] = dbc.lastrowid
#				print "Found new type", type, tid
#			
#			if games.has_key(game) :
#				gid = games[game]
#			else :
#				dbc.execute('insert into games values (null, ?)', (game,))
#				gid = games[game] = dbc.lastrowid
#				print "Found new game", game, gid
#			
#			if authors.has_key(author) :
#				aid = authors[author]
#			else :
#				dbc.execute('insert into authors values (null, ?)', (author,))
#				aid = authors[author] = dbc.lastrowid
#				print "Found new author", author, aid
#			
#			dbc.execute('insert into songs values (null, ?,?,?,?)', (name, aid, gid, tid))
			#print name, author, game, type
			dbc.execute('insert into songs2 values (null, ?,?,?,?)', (data, name, author, "")) #"FORMAT='" + type + "'"))
		except :
			print "Could not insert ",data, name, author
			raise
	
	db.commit();
	dbc.execute('PRAGMA temp_store = MEMORY;');
	dbc.execute('create table songs (_id INTEGER PRIMARY KEY, path TEXT, title TEXT, composer TEXT, metadata TEXT)')
	dbc.execute('insert into songs (path,title,composer,metadata) select path,title,composer,metadata from songs2 order by composer');
	dbc.execute('drop table songs2');

	db.commit();
	
if __name__ == "__main__":
	main(sys.argv[1:])
