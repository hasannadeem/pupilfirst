class AddConfirmedAtToConnectRequest < ActiveRecord::Migration[4.2]
  def change
    add_column :connect_requests, :confirmed_at, :datetime
  end
end
