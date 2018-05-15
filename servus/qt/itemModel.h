/* Copyright (c) 2015, Human Brain Project
 *                     Daniel.Nachbaur@epfl.ch
 *
 * This file is part of Servus <https://github.com/HBPVIS/Servus>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef SERVUSQT_ITEMMODEL
#define SERVUSQT_ITEMMODEL

#include <servus/types.h>

#include <memory> // std::unique_ptr
#include <QAbstractItemModel> // base class
#include <servus/listener.h>
#include <servus/servus.h>

#include <QTimer>

namespace servus
{
namespace qt
{

/**
 * An item model on top of a Servus service, to be used in a Qt item view.
 *
 * The model is represented as a hierarchy with two levels where the first level
 * containts one row per discovered instance of the service, and the second
 * level contains one row per announced key-value pair.
 *
 * The model itself sets the given Servus instance into the browsing state and
 * asynchronously browses for new and/or deleted instances every 100ms.
 *
 * @version 1.2
 */
class ItemModel final : public QAbstractItemModel
{
public:
    /**
     * Construct a new model by filling it with the current discovered instances
     * and put the service into browsing state.
     *
     * @param service the mutable service instance that the model represents
     * @param parent optional parent for memory ownership
     * @version 1.2
     */
  ItemModel( Servus& service, QObject* parent = nullptr )
    : QAbstractItemModel( parent )
    , _impl(  service, *this )
  {
  }

    /** Destruct the model and reset the service back to non-browsing state. */
    virtual ~ItemModel()
    {
    }

    /** Mandatory override of QAbstractItemModel::index. */
    QModelIndex index( int row, int column,
                       const QModelIndex& parent_ = QModelIndex( )) const override
    {
      if( !hasIndex( row, column, parent_ ))
        return QModelIndex();

      QObject* parentItem;
      if( !parent_.isValid( ))
        parentItem = _impl.rootItem.get();
      else
        parentItem = static_cast< QObject* >( parent_.internalPointer( ));

      QObject* childItem = parentItem->children()[row];
      if( childItem )
        return createIndex( row, column, childItem );
      return QModelIndex();
    }

    /**
     * Mandatory override of QAbstractItemModel::parent.
     *
     * If index points to a key-value item, the parent will be the corresponding
     * instance. If index points to an instance, the parent will be
     * QModelIndex().
     */
    QModelIndex parent( const QModelIndex& index_ ) const override
    {
      if( !index_.isValid( ))
        return QModelIndex();

      QObject* childItem = static_cast< QObject* >( index_.internalPointer( ));
      QObject* parentItem = childItem->parent();

      if( parentItem == _impl.rootItem.get( ))
        return QModelIndex();

      if( !parentItem->parent( ))
        return createIndex( 0, 0, parentItem );
      const int row = parentItem->parent()->children().indexOf( parentItem );
      return createIndex( row, 0, parentItem );
    }

    /**
     * Mandatory override of QAbstractItemModel::rowCount.
     *
     * If index points is QModelIndex(), the row count will be the number of
     * discovered instances. If index points to an instance item, the row count
     * will be the number of announced key-value items. If index points to a
     * key-value item, the row count will always be 0.
     */

    int rowCount( const QModelIndex& parent_ = QModelIndex( )) const override
    {
      QObject* parentItem;
      if( !parent_.isValid( ))
        parentItem = _impl.rootItem.get();
      else
        parentItem = static_cast< QObject* >( parent_.internalPointer( ));

      return parentItem->children().size();
    }

    /**
     * Mandatory override of QAbstractItemModel::columnCount.
     *
     * Independent of index, the column count will always be 1.
     */

    int columnCount( const QModelIndex& index_ = QModelIndex( )) const override
    {
      Q_UNUSED( index_ );
      return 1;
    }

    /**
     * Mandatory override of QAbstractItemModel::data.
     *
     * If index points to an instance item, the returned data for
     * Qt::DisplayRole will be the instance name, and for Qt::ToolTipRole and
     * Qt::UserRole the data will be the hostname. If index points to a
     * key-value item, the returned data for Qt::DisplayRole will be a string in
     * the format "key = value". For any other index and/or role, the returned
     * data will be QVariant().
     */
    QVariant data( const QModelIndex& index_,
                   int role = Qt::DisplayRole ) const override
    {
      if( !index_.isValid( ))
        return QVariant();

      QObject* item = static_cast< QObject* >( index_.internalPointer( ));

      switch( role )
      {
      case Qt::DisplayRole:
        return item->objectName();
      case Qt::ToolTipRole:
      case Qt::UserRole:
        if( item->children().isEmpty( ))
          return QVariant();
        return QString::fromStdString(
              _impl.service.getHost( item->objectName().toStdString( )));
      default:
        return QVariant();
      }
    }

    /**
     * Optional override of QAbstractItemModel::headerData.
     *
     * If orientation is Qt::Horizontal and role is Qt::DisplayRole, the
     * returned data will be a string in the format
     * "Instances for <service-name>". For any other input, the returned data
     * will be QVariant().
     */
    QVariant headerData( int section, Qt::Orientation orientation,
                         int role ) const override
    {
      Q_UNUSED( section );
      if( orientation == Qt::Horizontal && role == Qt::DisplayRole )
        return _impl.rootItem->objectName();

      return QVariant();
    }

private:
    class Impl : public Listener
    {
    public:
        Impl( Servus& service_, ItemModel& parent_ )
            : parent( parent_ )
            , service( service_ )
            , rootItem( new QObject( ))
        {
            rootItem->setObjectName( QString( "Instances for %1" )
                                .arg( QString::fromStdString( service.getName( ))));

            for( const std::string& i : service.getInstances( ))
                _addInstanceItem( QString::fromStdString( i ));

            service.addListener( this );
            service.beginBrowsing( Interface::IF_ALL );

            browseTimer.connect( &browseTimer, &QTimer::timeout,
                                 [this]() { service.browse( 0 ); } );
            browseTimer.start( 100 );
        }

        ~Impl()
        {
            browseTimer.stop();
            service.removeListener( this );
            service.endBrowsing();
        }

        void instanceAdded( const std::string& instance ) final override
        {
            const QString& qstr = QString::fromStdString( instance );
            if( rootItem->findChild< QObject* >( qstr ))
                return;

            parent.beginInsertRows( QModelIndex(), rootItem->children().size(),
                                                   rootItem->children().size( ));
            _addInstanceItem( qstr );
            parent.endInsertRows();
        }

        void instanceRemoved( const std::string& instance ) final override
        {
            const QString& qstr = QString::fromStdString( instance );
            QObject* child = rootItem->findChild< QObject* >( qstr );
            if( !child )
                return;

            const QObjectList& children = rootItem->children();
            const int childIdx = children.indexOf( child );
            parent.beginRemoveRows( QModelIndex(), childIdx, childIdx );
            _removeInstanceItem( qstr );
            parent.endRemoveRows();
        }

        ItemModel& parent;
        Servus& service;
        std::unique_ptr< QObject > rootItem;
        QTimer browseTimer;

    private:
        void _addInstanceItem( const QString& instance )
        {
            const std::string& instanceStr = instance.toStdString();
            QObject* instanceItem = new QObject( rootItem.get( ));
            instanceItem->setObjectName( instance );
            const Strings& keys = service.getKeys( instanceStr );
            for( const std::string& key : keys )
            {
                const QString data = QString( "%1 = %2" )
                    .arg( QString::fromStdString( key ))
                    .arg( QString::fromStdString( service.get( instanceStr, key )));
                QObject* kvItem = new QObject( instanceItem );
                kvItem->setObjectName( data );
            }
        }

        void _removeInstanceItem( const QString& instance )
        {
            QObject* child = rootItem->findChild< QObject* >( instance );
            delete child;
        }
    };

    Impl _impl;
};

}
}

#endif // SERVUSQT_ITEMMODEL
